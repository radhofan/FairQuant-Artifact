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

static const char* workclass_map[] = {
    "Private", "Self-emp-not-inc", "Self-emp-inc", "Federal-gov",
    "Local-gov", "State-gov", "Without-pay", "Never-worked"
};

static const char* education_map[] = {
    "Bachelors", "Some-college", "11th", "HS-grad", "Prof-school",
    "Assoc-acdm", "Assoc-voc", "9th", "7th-8th", "12th", "Masters",
    "1st-4th", "10th", "Doctorate", "5th-6th", "Preschool"
};

static const char* marital_status_map[] = {
    "Married-civ-spouse", "Divorced", "Never-married", "Separated",
    "Widowed", "Married-spouse-absent", "Married-AF-spouse"
};

static const char* occupation_map[] = {
    "Tech-support", "Craft-repair", "Other-service", "Sales", "Exec-managerial",
    "Prof-specialty", "Handlers-cleaners", "Machine-op-inspct", "Adm-clerical",
    "Farming-fishing", "Transport-moving", "Priv-house-serv", "Protective-serv",
    "Armed-Forces"
};

static const char* relationship_map[] = {
    "Wife", "Own-child", "Husband", "Not-in-family", "Other-relative", "Unmarried"
};

static const char* sex_map[] = { "Female", "Male" };

static const char* race_map[] = {
    "White", "Asian-Pac-Islander", "Amer-Indian-Eskimo", "Other", "Black"
};

static const char* native_country_map[] = {
    "United-States", "Cambodia", "England", "Puerto-Rico", "Canada",
    "Germany", "Outlying-US(Guam-USVI-etc)", "India", "Japan", "Greece",
    "South", "China", "Cuba", "Iran", "Honduras", "Philippines", "Italy",
    "Poland", "Jamaica", "Vietnam", "Mexico", "Portugal", "Ireland",
    "France", "Dominican-Republic", "Laos", "Ecuador", "Taiwan", "Haiti",
    "Columbia", "Hungary", "Guatemala", "Nicaragua", "Scotland", "Thailand",
    "Yugoslavia", "El-Salvador", "Trinadad&Tobago", "Peru", "Hong",
    "Holand-Netherlands"
};

const char* decode_bin(float value, float min_val, float max_val, int n_bins) {
    static char buffer[32];
    int idx = (int)round(value);
    float bin_width = (max_val - min_val) / n_bins;
    float midpoint = min_val + (idx + 0.5f) * bin_width;
    snprintf(buffer, sizeof(buffer), "%d", (int)midpoint);
    return buffer;
}

const char* decode_feature(int feature_index, float value) {
    int idx = (int)round(value);
    switch (feature_index) {
        case 1: return workclass_map[idx];
        case 2: return education_map[idx];
        case 4: return marital_status_map[idx];
        case 5: return occupation_map[idx];
        case 6: return relationship_map[idx];
        case 7: return race_map[idx];
        case 8: return sex_map[idx];
        case 9: return decode_bin(value, 0.0f, 100000.0f, 20);  // capital-gain (real max ≈ 99999)
        case 10: return decode_bin(value, 0.0f, 4356.0f, 20);   // capital-loss (real max ≈ 4356)
        case 12: return native_country_map[idx];
        default: {
            static char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.0f", value);
            return buffer;
        }
    }
}


// Main check_adv function
int check_adv(struct NNet* nnet, struct Subproblem *subp) {
    static int counterexample_count = 0;
    static FILE* ce_file = NULL;

    // static const char* feature_names[] = {
    //     "age", "workclass", "fnlwgt", "education", "education-num",
    //     "marital-status", "occupation", "relationship", "sex", "race",
    //     "capital-gain", "capital-loss", "hours-per-week", "native-country"
    // };

    static const char* feature_names[] = {
        "age",
        "workclass",
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

    if (ce_file == NULL) {
        ce_file = fopen("FairQuant-Artifact/FairQuant/counterexamples.csv", "w");
        if (!ce_file) {
            printf("Failed to open counterexamples.csv\n");
            return 0;
        }

        fprintf(ce_file, "CE_ID,PA,");
        for (int i = 0; i < nnet->inputSize; i++) {
            fprintf(ce_file, "%s,", feature_names[i]);
        }
        fprintf(ce_file, "Output,Decision\n");
        fflush(ce_file);
    }

    float a0[nnet->inputSize];
    float a1[nnet->inputSize];
    struct Matrix adv0 = {a0, 1, nnet->inputSize};
    struct Matrix adv1 = {a1, 1, nnet->inputSize};

    int counterexample = 0;

    for (int n = 0; n < 10; n++) {
        for (int i = 0; i < nnet->inputSize; i++) {
            if (i == nnet->sens_feature_idx) {
                a0[i] = nnet->mins[i];  // PA = 0
                a1[i] = nnet->maxes[i]; // PA = 1
            } else {
                float lower = subp->input.lower_matrix.data[i];
                float upper = subp->input.upper_matrix.data[i];
                float middle = lower + ((float)n / 10.0f) * (upper - lower);
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

        int out0Pos = output0.data[0] > 0;
        int out1Pos = output1.data[0] > 0;

        if (out0Pos != out1Pos) {
            counterexample_count++;

            // PA = 0
            fprintf(ce_file, "%d,0,", counterexample_count);
            for (int i = 0; i < nnet->inputSize; i++) {
                fprintf(ce_file, "%s,", decode_feature(i, a0[i]));
            }
            fprintf(ce_file, "%.6f,%s\n", output0.data[0], out0Pos ? "POSITIVE" : "NEGATIVE");

            // PA = 1
            fprintf(ce_file, "%d,1,", counterexample_count);
            for (int i = 0; i < nnet->inputSize; i++) {
                fprintf(ce_file, "%s,", decode_feature(i, a1[i]));
            }
            fprintf(ce_file, "%.6f,%s\n", output1.data[0], out1Pos ? "POSITIVE" : "NEGATIVE");

            fflush(ce_file);
            counterexample++;
            return 1; 
        }
    }

    // if(counterexample == 0){
    //     // fprintf(stdout, "TIDAK ADA\n");
    // }

    // return counterexample; // No counterexample found
    return 0;
}