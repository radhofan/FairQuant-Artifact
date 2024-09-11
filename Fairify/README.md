# Fairify

This repository contains the source code, benchmark models, and datasets for the paper - **"Fairify: Fairness Verification of Neural Networks"**, appeared in ICSE 2023 at Melbourne, Australia.

Original repository can be found [here](https://github.com/sumonbis/Fairify)

### Authors
* Sumon Biswas, Carnegie Mellon University (sumonb@cs.cmu.edu)
* Hridesh Rajan, Iowa State University (hridesh@iastate.edu)

**PDF** https://arxiv.org/abs/2212.06140

**DOI:** This artifact is also published in Zenodo:
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.7579939.svg)](https://doi.org/10.5281/zenodo.7579939)

![The problem tackled by Fairify](./problem.jpg)

## Index (modified for FairQuant)
> 1. [Models](../models/)
> 2. Datasets
  >> * [German Credit (GC)](../data/german)
  >> * [Adult Census (AC)](../data/adult)
  >> * [Bank Marketing (BM)](../data/bank)
  >> * [Compas Recidivism](../data/compas)
> 3. Verification source code
  >> * [Verify models](./src/)
  >> * [Utilities](./utils/)

## Installation

To run the tool, please refer to the [installation file](./INSTALL.md) for detailed instructions. 

### Cite the paper as
```
@inproceedings{biswas23fairify,
  author = {Sumon Biswas and Hridesh Rajan},
  title = {Fairify: Fairness Verification of Neural Networks},
  booktitle = {ICSE'23: The 45th International Conference on Software Engineering},
  location = {Melbourne, Australia},
  month = {May 14-May 20},
  year = {2023},
  entrysubtype = {conference}
}
```
