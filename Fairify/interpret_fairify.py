# script usage
# interpret_fairify.py <dataset> <csv_file>

import pandas as pd
import argparse

parser = argparse.ArgumentParser(description='Interpret the output of Fairify')
parser.add_argument('dataset', type=str, help='The dataset name')
parser.add_argument('csv_file', type=str, help='The csv file containing the output of Fairify')

args = parser.parse_args()
dataset = args.dataset
csv_file = args.csv_file

print("Reading {} data from {}".format(dataset, csv_file))

data = pd.read_csv(csv_file)
# print(data.columns)
# print(data.head())

# The last row contains the number of partitions that were SAT, UNSAT, or UNK
SAT_count = data.iloc[-1]["SAT_count"]
UNSAT_count = data.iloc[-1]["UNSAT_count"]
UNK_count = data.iloc[-1]["UNK_count"]

# The total number of partitions as defined in the paper
if dataset == "BM":
    total_partitions = 500
elif dataset == "GC":
    total_partitions = 201
elif dataset == "AC":
    total_partitions = 16000
elif dataset == "compas":
    total_partitions = 20
    total_individuals = 2496 # race (in paper), sex, or age (2*2*2*2*2*2*39)
else:
    raise ValueError("Invalid dataset")

# UNSAT partition means that the entire partition is Certified
# We report the percentage of partitions that are Certified
print("Cer (%) = {:.2f}".format(UNSAT_count/total_partitions*100))

# SAT partition means that they found a single counterexample for the partition. This is counted as #Cex = 1
# But because this is a single counterexample, it does not mean that the entire partition is Falsified
# The tool does not provide further information about how much of the partition is actually Certified
# So, we consider it as Undecided, alongside the UNK partitions
# Since the tool may terminate early due to timeout and not report on all the partitions, 
# we subtract the the number of UNSAT partitions from the total partitions to get the total number of partitions that are either SAT or UNK
print("Und (%) = {:.2f}".format((total_partitions-UNSAT_count)/total_partitions*100))


print("#Cex = {}".format(SAT_count))

# In the case of compas, we actually count the number of Cex and divide it up by the number of instances 
# and count it as Falsified because the dataset is small
if dataset != "compas":
    print("Fals (%) = 0")
else:
    print("Fals (%) = {:.2f}".format(SAT_count/total_individuals*100))