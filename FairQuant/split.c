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

// Structure to hold original feature mappings
struct FeatureDecoder {
    // Categorical feature mappings
    char* workclass_values[8];
    char* education_values[16];
    char* marital_status_values[7];
    char* occupation_values[14];
    char* relationship_values[6];
    char* race_values[5];
    char* sex_values[2];
    char* native_country_values[42];
};

// Initialize the decoder with your dataset's mappings
void init_feature_decoder(struct FeatureDecoder* decoder) {
    // Workclass mapping
    decoder->workclass_values[0] = "Private";
    decoder->workclass_values[1] = "Self-emp-not-inc";
    decoder->workclass_values[2] = "Self-emp-inc";
    decoder->workclass_values[3] = "Federal-gov";
    decoder->workclass_values[4] = "Local-gov";
    decoder->workclass_values[5] = "State-gov";
    decoder->workclass_values[6] = "Without-pay";
    decoder->workclass_values[7] = "Never-worked";
    
    // Education mapping
    decoder->education_values[0] = "Bachelors";
    decoder->education_values[1] = "Some-college";
    decoder->education_values[2] = "11th";
    decoder->education_values[3] = "HS-grad";
    decoder->education_values[4] = "Prof-school";
    decoder->education_values[5] = "Assoc-acdm";
    decoder->education_values[6] = "Assoc-voc";
    decoder->education_values[7] = "9th";
    decoder->education_values[8] = "7th-8th";
    decoder->education_values[9] = "12th";
    decoder->education_values[10] = "Masters";
    decoder->education_values[11] = "1st-4th";
    decoder->education_values[12] = "10th";
    decoder->education_values[13] = "Doctorate";
    decoder->education_values[14] = "5th-6th";
    decoder->education_values[15] = "Preschool";
    
    // Marital status mapping
    decoder->marital_status_values[0] = "Married-civ-spouse";
    decoder->marital_status_values[1] = "Divorced";
    decoder->marital_status_values[2] = "Never-married";
    decoder->marital_status_values[3] = "Separated";
    decoder->marital_status_values[4] = "Widowed";
    decoder->marital_status_values[5] = "Married-spouse-absent";
    decoder->marital_status_values[6] = "Married-AF-spouse";
    
    // Occupation mapping
    decoder->occupation_values[0] = "Tech-support";
    decoder->occupation_values[1] = "Craft-repair";
    decoder->occupation_values[2] = "Other-service";
    decoder->occupation_values[3] = "Sales";
    decoder->occupation_values[4] = "Exec-managerial";
    decoder->occupation_values[5] = "Prof-specialty";
    decoder->occupation_values[6] = "Handlers-cleaners";
    decoder->occupation_values[7] = "Machine-op-inspct";
    decoder->occupation_values[8] = "Adm-clerical";
    decoder->occupation_values[9] = "Farming-fishing";
    decoder->occupation_values[10] = "Transport-moving";
    decoder->occupation_values[11] = "Priv-house-serv";
    decoder->occupation_values[12] = "Protective-serv";
    decoder->occupation_values[13] = "Armed-Forces";
    
    // Relationship mapping
    decoder->relationship_values[0] = "Wife";
    decoder->relationship_values[1] = "Own-child";
    decoder->relationship_values[2] = "Husband";
    decoder->relationship_values[3] = "Not-in-family";
    decoder->relationship_values[4] = "Other-relative";
    decoder->relationship_values[5] = "Unmarried";
    
    // Race mapping
    decoder->race_values[0] = "White";
    decoder->race_values[1] = "Asian-Pac-Islander";
    decoder->race_values[2] = "Amer-Indian-Eskimo";
    decoder->race_values[3] = "Other";
    decoder->race_values[4] = "Black";
    
    // Sex mapping
    decoder->sex_values[0] = "Female";
    decoder->sex_values[1] = "Male";
    
    // Native country mapping
    decoder->native_country_values[0] = "United-States";
    decoder->native_country_values[1] = "Cambodia";
    decoder->native_country_values[2] = "England";
    decoder->native_country_values[3] = "Puerto-Rico";
    decoder->native_country_values[4] = "Canada";
    decoder->native_country_values[5] = "Germany";
    decoder->native_country_values[6] = "Outlying-US(Guam-USVI-etc)";
    decoder->native_country_values[7] = "India";
    decoder->native_country_values[8] = "Japan";
    decoder->native_country_values[9] = "Greece";
    decoder->native_country_values[10] = "South";
    decoder->native_country_values[11] = "China";
    decoder->native_country_values[12] = "Cuba";
    decoder->native_country_values[13] = "Iran";
    decoder->native_country_values[14] = "Honduras";
    decoder->native_country_values[15] = "Philippines";
    decoder->native_country_values[16] = "Italy";
    decoder->native_country_values[17] = "Poland";
    decoder->native_country_values[18] = "Jamaica";
    decoder->native_country_values[19] = "Vietnam";
    decoder->native_country_values[20] = "Mexico";
    decoder->native_country_values[21] = "Portugal";
    decoder->native_country_values[22] = "Ireland";
    decoder->native_country_values[23] = "France";
    decoder->native_country_values[24] = "Dominican-Republic";
    decoder->native_country_values[25] = "Laos";
    decoder->native_country_values[26] = "Ecuador";
    decoder->native_country_values[27] = "Taiwan";
    decoder->native_country_values[28] = "Haiti";
    decoder->native_country_values[29] = "Columbia";
    decoder->native_country_values[30] = "Hungary";
    decoder->native_country_values[31] = "Guatemala";
    decoder->native_country_values[32] = "Nicaragua";
    decoder->native_country_values[33] = "Scotland";
    decoder->native_country_values[34] = "Thailand";
    decoder->native_country_values[35] = "Yugoslavia";
    decoder->native_country_values[36] = "El-Salvador";
    decoder->native_country_values[37] = "Trinadad&Tobago";
    decoder->native_country_values[38] = "Peru";
    decoder->native_country_values[39] = "Hong";
    decoder->native_country_values[40] = "Holand-Netherlands";
    decoder->native_country_values[41] = "Unknown";
}

// Function to decode a single feature value back to original
void decode_feature_value(int feature_idx, float encoded_value, struct FeatureDecoder* decoder, 
                         char* output_buffer, int buffer_size) {
    int category_idx;
    
    switch(feature_idx) {
        case 0: // age - continuous
            snprintf(output_buffer, buffer_size, "%.1f", encoded_value);
            break;
            
        case 1: // workclass - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 8) {
                snprintf(output_buffer, buffer_size, "%s", decoder->workclass_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 2: // fnlwgt - continuous
            snprintf(output_buffer, buffer_size, "%.0f", encoded_value);
            break;
            
        case 3: // education - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 16) {
                snprintf(output_buffer, buffer_size, "%s", decoder->education_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 4: // education-num - continuous
            snprintf(output_buffer, buffer_size, "%.0f", encoded_value);
            break;
            
        case 5: // marital-status - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 7) {
                snprintf(output_buffer, buffer_size, "%s", decoder->marital_status_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 6: // occupation - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 14) {
                snprintf(output_buffer, buffer_size, "%s", decoder->occupation_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 7: // relationship - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 6) {
                snprintf(output_buffer, buffer_size, "%s", decoder->relationship_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 8: // race - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 5) {
                snprintf(output_buffer, buffer_size, "%s", decoder->race_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 9: // sex - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 2) {
                snprintf(output_buffer, buffer_size, "%s", decoder->sex_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        case 10: // capital-gain - continuous
            snprintf(output_buffer, buffer_size, "%.0f", encoded_value);
            break;
            
        case 11: // capital-loss - continuous
            snprintf(output_buffer, buffer_size, "%.0f", encoded_value);
            break;
            
        case 12: // hours-per-week - continuous
            snprintf(output_buffer, buffer_size, "%.0f", encoded_value);
            break;
            
        case 13: // native-country - categorical
            category_idx = (int)round(encoded_value);
            if (category_idx >= 0 && category_idx < 42) {
                snprintf(output_buffer, buffer_size, "%s", decoder->native_country_values[category_idx]);
            } else {
                snprintf(output_buffer, buffer_size, "Unknown");
            }
            break;
            
        default:
            snprintf(output_buffer, buffer_size, "%.4f", encoded_value);
            break;
    }
}

// Updated check_adv function with decoding
int check_adv(struct NNet* nnet, struct Subproblem *subp)
{
    static int counterexample_count = 0;  // Keep track of total counterexamples found
    static FILE* ce_file = NULL;
    static struct FeatureDecoder decoder;
    static int decoder_initialized = 0;
    
    // Feature names for the Adult dataset
    static const char* feature_names[] = {
        "age",
        "workclass", 
        "fnlwgt",
        "education",
        "education-num",
        "marital-status",
        "occupation",
        "relationship",
        "race",
        "sex",
        "capital-gain",
        "capital-loss",
        "hours-per-week",
        "native-country"
    };
   
    // Initialize decoder on first call
    if (!decoder_initialized) {
        init_feature_decoder(&decoder);
        decoder_initialized = 1;
    }
   
    // Open file on first call
    if (ce_file == NULL) {
        ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
        if (ce_file != NULL) {
            // Write CSV header with actual feature names
            fprintf(ce_file, "CE_ID,Sample_ID,PA,");
            for (int i=0; i<nnet->inputSize; i++) {
                fprintf(ce_file, "%s,", feature_names[i]);
            }
            fprintf(ce_file, "Output,Decision\n");
        }
    }
   
    float a0[nnet->inputSize];
    float a1[nnet->inputSize];
    struct Matrix adv0 = {a0, 1, nnet->inputSize};
    struct Matrix adv1 = {a1, 1, nnet->inputSize};
   
    // trying 10 different samples
    for (int n=0; n<10; n++){
        // concrete data point is gender + some point for all other features
        for (int i=0; i<nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx){
                a0[i] = nnet->mins[i];  //for PA=0
                a1[i] = nnet->maxes[i]; //for PA=1
            }
            else {
                // Fixed sampling logic using floating point arithmetic
                float upper = subp->input.upper_matrix.data[i];
                float lower = subp->input.lower_matrix.data[i];
                float middle = lower + (float)n * (upper - lower) / 10.0f;
                a0[i] = middle;
                a1[i] = middle;
            }
        }
       
        float out0[nnet->outputSize];
        float out1[nnet->outputSize];
        struct Matrix output0 = {out0, nnet->outputSize, 1};
        struct Matrix output1 = {out1, nnet->outputSize, 1};
       
        forward_prop(nnet, &adv0, &output0);
        forward_prop(nnet, &adv1, &output1);
       
        // for sigmoid, one output node
        int out0Pos = (output0.data[0] > 0);
        int out1Pos = (output1.data[0] > 0);
       
        // at any point, this sample is adv, then we can return here
        if (out0Pos != out1Pos) {
            counterexample_count++;
           
            // Save to CSV file with decoded values
            if (ce_file != NULL) {
                char decoded_value[256];  // Buffer for decoded feature values
                
                // Row for PA=0 group
                fprintf(ce_file, "%d,%d,0,", counterexample_count, n+1);
                for (int i=0; i<nnet->inputSize; i++) {
                    decode_feature_value(i, a0[i], &decoder, decoded_value, sizeof(decoded_value));
                    fprintf(ce_file, "%s,", decoded_value);
                }
                fprintf(ce_file, "%.6f,%s\n", output0.data[0],
                        out0Pos ? "POSITIVE" : "NEGATIVE");
               
                // Row for PA=1 group
                fprintf(ce_file, "%d,%d,1,", counterexample_count, n+1);
                for (int i=0; i<nnet->inputSize; i++) {
                    decode_feature_value(i, a1[i], &decoder, decoded_value, sizeof(decoded_value));
                    fprintf(ce_file, "%s,", decoded_value);
                }
                fprintf(ce_file, "%.6f,%s\n", output1.data[0],
                        out1Pos ? "POSITIVE" : "NEGATIVE");
               
                fflush(ce_file);  // Ensure data is written immediately
            }
           
            return 1;   //is_adv!
        }
    }
    return 0;
}