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

// Categorical feature mappings
char* workclass_map[] = {
    "Private", "Self-emp-not-inc", "Self-emp-inc",
    "Federal-gov", "Local-gov", "State-gov",
    "Without-pay", "Never-worked"
};

char* education_map[] = {
    "Preschool", "1st-4th", "5th-6th", "7th-8th",
    "9th", "10th", "11th", "12th", "HS-grad",
    "Prof-school", "Assoc-acdm", "Assoc-voc", "Some-college",
    "Bachelors", "Masters", "Doctorate"
};

char* marital_status_map[] = {
    "Married-civ-spouse", "Divorced", "Never-married",
    "Separated", "Widowed", "Married-spouse-absent",
    "Married-AF-spouse"
};

char* occupation_map[] = {
    "Tech-support", "Craft-repair", "Other-service", "Sales",
    "Exec-managerial", "Prof-specialty", "Handlers-cleaners",
    "Machine-op-inspct", "Adm-clerical", "Farming-fishing",
    "Transport-moving", "Priv-house-serv", "Protective-serv",
    "Armed-Forces"
};

char* relationship_map[] = {
    "Wife", "Own-child", "Husband", "Not-in-family",
    "Other-relative", "Unmarried"
};

char* race_map[] = {
    "White", "Asian-Pac-Islander", "Amer-Indian-Eskimo",
    "Other", "Black"
};

char* sex_map[] = {
    "Female", "Male"
};

char* native_country_map[] = {
    "United-States", "Cambodia", "England", "Puerto-Rico",
    "Canada", "Germany", "Outlying-US(Guam-USVI-etc)",
    "India", "Japan", "Greece", "South", "China", "Cuba",
    "Iran", "Honduras", "Philippines", "Italy", "Poland",
    "Jamaica", "Vietnam", "Mexico", "Portugal", "Ireland",
    "France", "Dominican-Republic", "Laos", "Ecuador",
    "Taiwan", "Haiti", "Columbia", "Hungary", "Guatemala",
    "Nicaragua", "Scotland", "Thailand", "Yugoslavia",
    "El-Salvador", "Trinadad&Tobago", "Peru", "Hong", "Holand-Netherlands"
};

const char* get_categorical_name(int idx, int val) {
    switch(idx) {
        case 1: return (val >= 0 && val < sizeof(workclass_map)/sizeof(char*)) ? workclass_map[val] : "Unknown";
        case 3: return (val >= 0 && val < sizeof(education_map)/sizeof(char*)) ? education_map[val] : "Unknown";
        case 5: return (val >= 0 && val < sizeof(marital_status_map)/sizeof(char*)) ? marital_status_map[val] : "Unknown";
        case 6: return (val >= 0 && val < sizeof(occupation_map)/sizeof(char*)) ? occupation_map[val] : "Unknown";
        case 7: return (val >= 0 && val < sizeof(relationship_map)/sizeof(char*)) ? relationship_map[val] : "Unknown";
        case 8: return (val >= 0 && val < sizeof(sex_map)/sizeof(char*)) ? sex_map[val] : "Unknown";
        case 9: return (val >= 0 && val < sizeof(race_map)/sizeof(char*)) ? race_map[val] : "Unknown";
        case 13: return (val >= 0 && val < sizeof(native_country_map)/sizeof(char*)) ? native_country_map[val] : "Unknown";
        default: return "Unknown";
    }
}

int check_adv(struct NNet* nnet, struct Subproblem *subp)
{
    static int counterexample_count = 0;  // Keep track of total counterexamples found
    static FILE* ce_file = NULL;

    // Feature names for the Adult dataset
    static const char* feature_names[] = {
        "age", "workclass", "fnlwgt", "education", "education-num",
        "marital-status", "occupation", "relationship", "race", "sex",
        "capital-gain", "capital-loss", "hours-per-week", "native-country"
    };

    // Open file on first call and write header
    if (ce_file == NULL) {
        ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
        if (ce_file != NULL) {
            // Write CSV header
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

    // trying 10 different samples
    for (int n=0; n<10; n++){

        // Construct inputs for both protected groups
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx){
                a0[i] = nnet->mins[i];  // PA=0
                a1[i] = nnet->maxes[i]; // PA=1
            }
            else {
                float upper = subp->input.upper_matrix.data[i];
                float lower = subp->input.lower_matrix.data[i];
                float middle = lower + ((float)n / 10.0f) * (upper - lower);

                // Round categorical features (after denormalization)
                a0[i] = middle;
                a1[i] = middle;
            }
        }

        // Create copies for denormalization (to avoid modifying original arrays)
        float a0_denorm[nnet->inputSize];
        float a1_denorm[nnet->inputSize];
        
        for (int i = 0; i < nnet->inputSize; i++) {
            a0_denorm[i] = a0[i];
            a1_denorm[i] = a1[i];
        }
        
        struct Matrix adv0_denorm = {a0_denorm, 1, nnet->inputSize};
        struct Matrix adv1_denorm = {a1_denorm, 1, nnet->inputSize};
        
        // Denormalize the inputs for display purposes
        // Use min-max denormalization: denormalized = normalized * (max - min) + min
        for (int i = 0; i < nnet->inputSize; i++) {
            a0_denorm[i] = a0_denorm[i] * (nnet->maxes[i] - nnet->mins[i]) + nnet->mins[i];
            a1_denorm[i] = a1_denorm[i] * (nnet->maxes[i] - nnet->mins[i]) + nnet->mins[i];
        }
        
        // Round categorical features after denormalization
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == 1 || i == 3 || i == 5 || i == 6 ||
                i == 7 || i == 8 || i == 9 || i == 13) {
                a0_denorm[i] = round(a0_denorm[i]);
                a1_denorm[i] = round(a1_denorm[i]);
            }
        }

        float out0[nnet->outputSize];
        float out1[nnet->outputSize];
        struct Matrix output0 = {out0, nnet->outputSize, 1};
        struct Matrix output1 = {out1, nnet->outputSize, 1};

        // Use original normalized inputs for forward propagation
        forward_prop(nnet, &adv0, &output0);
        forward_prop(nnet, &adv1, &output1);

        // Binary classification decision via threshold (e.g., sigmoid > 0)
        int out0Pos = (output0.data[0] > 0);
        int out1Pos = (output1.data[0] > 0);

        // If decisions differ, we have an adversarial example
        if (out0Pos != out1Pos) {
            counterexample_count++;

            // Save both PA=0 and PA=1 as separate rows in CSV
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
                fprintf(ce_file, "%.6f,%s\n",
                        output0.data[0],
                        out0Pos ? "POSITIVE" : "NEGATIVE");

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
                fprintf(ce_file, "%.6f,%s\n",
                        output1.data[0],
                        out1Pos ? "POSITIVE" : "NEGATIVE");

                fflush(ce_file);  // Flush immediately
            }

            return 1;   // Adversarial case found
        }
    }

    return 0; // No adversarial case found
}