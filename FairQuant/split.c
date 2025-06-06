/*
 -----------------------------------------------------------------
 ** Top contributors:
 **   Shiqi Wang and Suman Jana
 ** This file is part of the ReluVal project.
 ** Copyright (c) 2018-2019 by the authors listed in the file LICENSE
 ** and their institutional affiliations.
 ** All rights reserved.
 -----------------------------------------------------------------
 */

#include "split.h"

#define AVG_WINDOW 5
#define MAX_THREAD 56
#define MIN_DEPTH_PER_THREAD 5 

float rateCert = 0, rateFals = 0, rateAdv = 0, rateUncer = 1;

struct timeval start, curr, finish, last_finish;

/*
 * Check the existance of concrete adversarial examples
 * It takes in the network and input ranges.
 * If a concrete adversarial example is found,
 * return adv_found 1
 */
// int check_adv(struct NNet* nnet, struct Subproblem *subp)
// {

//     float a0[nnet->inputSize];
//     float a1[nnet->inputSize];
//     struct Matrix adv0 = {a0, 1, nnet->inputSize};
//     struct Matrix adv1 = {a1, 1, nnet->inputSize};

//     // trying 10 different samples
//     for (int n=0; n<10; n++){

//         // concrete data point is gender + some point for all other features
//         for (int i=0; i<nnet->inputSize; i++) {
//             if (i == nnet->sens_feature_idx){
//                 a0[i] = nnet->mins[i];  //for PA=0
//                 a1[i] = nnet->maxes[i]; //for PA=1
//             }
//             else {
//                 int upper = (int) subp->input.upper_matrix.data[i];
//                 int lower = (int) subp->input.lower_matrix.data[i];
//                 int middle = n*(lower+upper)/10; // floor

//                 a0[i] = (float) middle;
//                 a1[i] = (float) middle;
//             }
//         }

//         float out0[nnet->outputSize];
//         float out1[nnet->outputSize];
//         struct Matrix output0 = {out0, nnet->outputSize, 1};
//         struct Matrix output1 = {out1, nnet->outputSize, 1};

//         forward_prop(nnet, &adv0, &output0);
//         forward_prop(nnet, &adv1, &output1);

//         // for sigmoid, one output node
//         int out0Pos = (output0.data[0] > 0);
//         int out1Pos = (output1.data[0] > 0);

//         // at any point, this sample is adv, then we can return here
//         // and no need to check further
//         if (out0Pos != out1Pos) {
//             return 1;   //is_adv!
//         } 
//     }

//     // if we made it here, that means no adv sample was found
//     return 0;
// }


// You need to add these arrays with the ACTUAL original data bounds
// These should be the min/max values from your original dataset before normalization
float original_mins[] = {
    17.0,   // age
    0.0,    // workclass (categorical, 0-7)
    12285.0, // fnlwgt (example, check your actual data)
    0.0,    // education (categorical, 0-15)
    1.0,    // education-num
    0.0,    // marital-status (categorical, 0-6)
    0.0,    // occupation (categorical, 0-13)
    0.0,    // relationship (categorical, 0-5)
    0.0,    // race (categorical, 0-4)
    0.0,    // sex (categorical, 0-1)
    0.0,    // capital-gain
    0.0,    // capital-loss
    1.0,    // hours-per-week
    0.0     // native-country (categorical, 0-41)
};

float original_maxes[] = {
    90.0,   // age
    7.0,    // workclass
    1484705.0, // fnlwgt (example, check your actual data)
    15.0,   // education
    16.0,   // education-num
    6.0,    // marital-status
    13.0,   // occupation
    5.0,    // relationship
    4.0,    // race
    1.0,    // sex
    99999.0, // capital-gain (example)
    4356.0, // capital-loss (example)
    99.0,   // hours-per-week
    41.0    // native-country
};

int check_adv(struct NNet* nnet, struct Subproblem *subp)
{
    static int counterexample_count = 0;
    static FILE* ce_file = NULL;

    static const char* feature_names[] = {
        "age", "workclass", "fnlwgt", "education", "education-num",
        "marital-status", "occupation", "relationship", "race", "sex",
        "capital-gain", "capital-loss", "hours-per-week", "native-country"
    };

    if (ce_file == NULL) {
        ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
        if (ce_file != NULL) {
            fprintf(ce_file, "CE_ID,Sample_ID,PA,");
            for (int i = 0; i < nnet->inputSize; i++) {
                fprintf(ce_file, "%s,", feature_names[i]);
            }
            fprintf(ce_file, "Output,Decision\n");
            fflush(ce_file);
        } else {
            printf("Failed to open counterexamples.csv\n");
            return 0;
        }
    }

    float a0[nnet->inputSize];
    float a1[nnet->inputSize];
    struct Matrix adv0 = {a0, 1, nnet->inputSize};
    struct Matrix adv1 = {a1, 1, nnet->inputSize};

    for (int n = 0; n < 10; n++) {
        // Construct inputs for both protected groups
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx) {
                // For sensitive feature, use the normalized min/max values directly
                a0[i] = nnet->mins[i];  // PA=0 (normalized)
                a1[i] = nnet->maxes[i]; // PA=1 (normalized)
            } else {
                float upper = subp->input.upper_matrix.data[i];
                float lower = subp->input.lower_matrix.data[i];
                float middle = lower + ((float)n / 10.0f) * (upper - lower);
                a0[i] = middle;
                a1[i] = middle;
            }
        }

        // Create arrays for denormalized values
        float a0_denorm[nnet->inputSize];
        float a1_denorm[nnet->inputSize];
        
        // Denormalize using the original dataset bounds
        for (int i = 0; i < nnet->inputSize; i++) {
            // Use proper denormalization formula with original bounds
            a0_denorm[i] = a0[i] * (original_maxes[i] - original_mins[i]) + original_mins[i];
            a1_denorm[i] = a1[i] * (original_maxes[i] - original_mins[i]) + original_mins[i];
        }
        
        // Round categorical features after denormalization
        int categorical_features[] = {1, 3, 5, 6, 7, 8, 9, 13};
        int num_categorical = sizeof(categorical_features) / sizeof(int);
        
        for (int j = 0; j < num_categorical; j++) {
            int i = categorical_features[j];
            if (i < nnet->inputSize) {
                a0_denorm[i] = round(a0_denorm[i]);
                a1_denorm[i] = round(a1_denorm[i]);
                
                // Clamp to valid range for categorical features
                if (a0_denorm[i] < original_mins[i]) a0_denorm[i] = original_mins[i];
                if (a0_denorm[i] > original_maxes[i]) a0_denorm[i] = original_maxes[i];
                if (a1_denorm[i] < original_mins[i]) a1_denorm[i] = original_mins[i];
                if (a1_denorm[i] > original_maxes[i]) a1_denorm[i] = original_maxes[i];
            }
        }

        float out0[nnet->outputSize];
        float out1[nnet->outputSize];
        struct Matrix output0 = {out0, nnet->outputSize, 1};
        struct Matrix output1 = {out1, nnet->outputSize, 1};

        // Use original normalized inputs for forward propagation
        forward_prop(nnet, &adv0, &output0);
        forward_prop(nnet, &adv1, &output1);

        int out0Pos = (output0.data[0] > 0);
        int out1Pos = (output1.data[0] > 0);

        if (out0Pos != out1Pos) {
            counterexample_count++;

            if (ce_file != NULL) {
                // Row for PA=0
                fprintf(ce_file, "%d,%d,0,", counterexample_count, n+1);
                for (int i = 0; i < nnet->inputSize; i++) {
                    if (i == nnet->sens_feature_idx || 
                        i == 1 || i == 3 || i == 5 || i == 6 || 
                        i == 7 || i == 8 || i == 9 || i == 13) {
                        int val = (int)round(a0_denorm[i]);
                        const char* name = get_categorical_name(i, val);
                        fprintf(ce_file, "%s,", name);
                    } else {
                        fprintf(ce_file, "%.4f,", a0_denorm[i]);
                    }
                }
                fprintf(ce_file, "%.6f,%s\n", output0.data[0], out0Pos ? "POSITIVE" : "NEGATIVE");

                // Row for PA=1
                fprintf(ce_file, "%d,%d,1,", counterexample_count, n+1);
                for (int i = 0; i < nnet->inputSize; i++) {
                    if (i == nnet->sens_feature_idx || 
                        i == 1 || i == 3 || i == 5 || i == 6 || 
                        i == 7 || i == 8 || i == 9 || i == 13) {
                        int val = (int)round(a1_denorm[i]);
                        const char* name = get_categorical_name(i, val);
                        fprintf(ce_file, "%s,", name);
                    } else {
                        fprintf(ce_file, "%.4f,", a1_denorm[i]);
                    }
                }
                fprintf(ce_file, "%.6f,%s\n", output1.data[0], out1Pos ? "POSITIVE" : "NEGATIVE");

                fflush(ce_file);
            }

            return 1;
        }
    }

    return 0;
}