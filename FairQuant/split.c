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


int check_adv(struct NNet* nnet, struct Subproblem *subp)
{
    static int counterexample_count = 0;
    static FILE* ce_file = NULL;

    // Feature names in the exact order as they appear in the neural network
    static const char* feature_names[] = {
        "age", "workclass", "fnlwgt", "education", "education-num",
        "marital-status", "occupation", "relationship", "race", "sex",
        "capital-gain", "capital-loss", "hours-per-week", "native-country"
    };

    // Open CSV file and write headers
    if (ce_file == NULL) {
        ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
        if (ce_file != NULL) {
            // Write CSV header
            fprintf(ce_file, "CE_ID,PA,");
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

    // Generate 10 test samples
    for (int n = 0; n < 10; n++) {
        // Construct inputs for both protected groups
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx) {
                // For the sensitive feature (protected attribute)
                a0[i] = nnet->mins[i];  // PA=0
                a1[i] = nnet->maxes[i]; // PA=1
            } else {
                // For other features, interpolate within the subproblem bounds
                float upper = subp->input.upper_matrix.data[i];
                float lower = subp->input.lower_matrix.data[i];
                float middle = lower + ((float)n / 10.0f) * (upper - lower);
                a0[i] = middle;
                a1[i] = middle;
            }
        }

        // Forward propagation for both groups
        float out0[nnet->outputSize];
        float out1[nnet->outputSize];
        struct Matrix output0 = {out0, nnet->outputSize, 1};
        struct Matrix output1 = {out1, nnet->outputSize, 1};

        forward_prop(nnet, &adv0, &output0);
        forward_prop(nnet, &adv1, &output1);

        // Determine decisions (positive if output > 0)
        int out0Pos = (output0.data[0] > 0);
        int out1Pos = (output1.data[0] > 0);

        // Check if there's a difference in decisions (counterexample found)
        if (out0Pos != out1Pos) {
            counterexample_count++;

            if (ce_file != NULL) {
                // Write row for PA=0
                fprintf(ce_file, "%d,0,", counterexample_count);
                for (int i = 0; i < nnet->inputSize; i++) {
                    fprintf(ce_file, "%.4f,", a0[i]);
                }
                fprintf(ce_file, "%.6f,%s\n", output0.data[0], out0Pos ? "POSITIVE" : "NEGATIVE");

                // Write row for PA=1
                fprintf(ce_file, "%d,1,", counterexample_count);
                for (int i = 0; i < nnet->inputSize; i++) {
                    fprintf(ce_file, "%.4f,", a1[i]);
                }
                fprintf(ce_file, "%.6f,%s\n", output1.data[0], out1Pos ? "POSITIVE" : "NEGATIVE");

                fflush(ce_file);
            }

            return 1; // Counterexample found
        }
    }

    return 0; // No counterexample found
}