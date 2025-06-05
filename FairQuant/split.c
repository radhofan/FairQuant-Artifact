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

// int check_adv(struct NNet* nnet, struct Subproblem *subp)
// {
//     static int counterexample_count = 0;  // Keep track of total counterexamples found
//     static FILE* ce_file = NULL;
   
//     // Open file on first call
//     if (ce_file == NULL) {
//         ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
//         if (ce_file != NULL) {
//             // Write CSV header
//             fprintf(ce_file, "CE_ID,Sample_ID,");
//             for (int i=0; i<nnet->inputSize; i++) {
//                 fprintf(ce_file, "Feature_%d_PA0,Feature_%d_PA1,", i, i);
//             }
//             fprintf(ce_file, "Output_PA0,Output_PA1,Decision_PA0,Decision_PA1\n");
//         }
//     }
   
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
//         if (out0Pos != out1Pos) {
//             counterexample_count++;
           
//             // Save to CSV file silently
//             if (ce_file != NULL) {
//                 fprintf(ce_file, "%d,%d,", counterexample_count, n+1);
//                 for (int i=0; i<nnet->inputSize; i++) {
//                     fprintf(ce_file, "%.4f,%.4f,", a0[i], a1[i]);
//                 }
//                 fprintf(ce_file, "%.6f,%.6f,%s,%s\n",
//                         output0.data[0], output1.data[0],
//                         out0Pos ? "POSITIVE" : "NEGATIVE",
//                         out1Pos ? "POSITIVE" : "NEGATIVE");
//                 fflush(ce_file);  // Ensure data is written immediately
//             }
           
//             return 1;   //is_adv!
//         }
//     }
//     return 0;
// }

// int check_adv(struct NNet* nnet, struct Subproblem *subp)
// {
//     static int counterexample_count = 0;  // Keep track of total counterexamples found
//     static FILE* ce_file = NULL;
    
//     // Feature names for the Adult dataset
//     static const char* feature_names[] = {
//         "age",
//         "workclass", 
//         "fnlwgt",
//         "education",
//         "education-num",
//         "marital-status",
//         "occupation",
//         "relationship",
//         "race",
//         "sex",
//         "capital-gain",
//         "capital-loss",
//         "hours-per-week",
//         "native-country"
//     };
   
//     // Open file on first call
//     if (ce_file == NULL) {
//         ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
//         if (ce_file != NULL) {
//             // Write CSV header with actual feature names
//             fprintf(ce_file, "CE_ID,Sample_ID,PA,");
//             for (int i=0; i<nnet->inputSize; i++) {
//                 fprintf(ce_file, "%s,", feature_names[i]);
//             }
//             fprintf(ce_file, "Output,Decision\n");
//         }
//     }
   
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
//                 // Fixed sampling logic using floating point arithmetic
//                 float upper = subp->input.upper_matrix.data[i];
//                 float lower = subp->input.lower_matrix.data[i];
//                 float middle = lower + (float)n * (upper - lower) / 10.0f;
//                 a0[i] = middle;
//                 a1[i] = middle;
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
//         if (out0Pos != out1Pos) {
//             counterexample_count++;
           
//             // Save to CSV file silently
//             if (ce_file != NULL) {
//                 // Write two rows - one for each protected attribute group
               
//                 // Row for PA=0 group
//                 fprintf(ce_file, "%d,%d,0,", counterexample_count, n+1);
//                 for (int i=0; i<nnet->inputSize; i++) {
//                     fprintf(ce_file, "%.4f,", a0[i]);
//                 }
//                 fprintf(ce_file, "%.6f,%s\n", output0.data[0],
//                         out0Pos ? "POSITIVE" : "NEGATIVE");
               
//                 // Row for PA=1 group
//                 fprintf(ce_file, "%d,%d,1,", counterexample_count, n+1);
//                 for (int i=0; i<nnet->inputSize; i++) {
//                     fprintf(ce_file, "%.4f,", a1[i]);
//                 }
//                 fprintf(ce_file, "%.6f,%s\n", output1.data[0],
//                         out1Pos ? "POSITIVE" : "NEGATIVE");
               
//                 fflush(ce_file);  // Ensure data is written immediately
//             }
           
//             return 1;   //is_adv!
//         }
//     }
//     return 0;
// }


int check_adv(struct NNet* nnet, struct Subproblem *subp)
{
    static int counterexample_count = 0;  // Total CEs found so far
    static FILE* ce_file = NULL;

    // Feature names for the Adult dataset (or any other dataset you use)
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

    // Try 10 different samples
    for (int n = 0; n < 10; n++) {

        // Construct inputs for both protected groups
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx) {
                a0[i] = nnet->mins[i];  // PA=0
                a1[i] = nnet->maxes[i]; // PA=1
            } else {
                float upper = subp->input.upper_matrix.data[i];
                float lower = subp->input.lower_matrix.data[i];
                float middle = lower + ((float)n / 10.0f) * (upper - lower);

                a0[i] = middle;
                a1[i] = middle;
            }
        }

        // Forward pass for both inputs
        float out0[nnet->outputSize];
        float out1[nnet->outputSize];
        struct Matrix output0 = {out0, nnet->outputSize, 1};
        struct Matrix output1 = {out1, nnet->outputSize, 1};

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
                    fprintf(ce_file, "%.4f,", a0[i]);
                }
                fprintf(ce_file, "%.6f,%s\n",
                        output0.data[0],
                        out0Pos ? "POSITIVE" : "NEGATIVE");

                // Row for PA=1
                fprintf(ce_file, "%d,%d,1,", counterexample_count, n+1);
                for (int i = 0; i < nnet->inputSize; i++) {
                    fprintf(ce_file, "%.4f,", a1[i]);
                }
                fprintf(ce_file, "%.6f,%s\n",
                        output1.data[0],
                        out1Pos ? "POSITIVE" : "NEGATIVE");

                fflush(ce_file); // Flush immediately
            }

            return 1; // Adversarial case found
        }
    }

    return 0; // No adversarial case found
}