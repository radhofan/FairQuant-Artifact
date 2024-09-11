# Installation and Usage

To run Fairify, we need to install Python 3 environment. The current version has been tested on Python 3.7. It is recommended to install Python virtual environment for the tool. Furthermore, we used bash shell scripts to automate running benchmark and Python scripts. Below are step-by-step instructions to setup environment and run the tool. 

### Environment Setup

1. First, create a Python virtual environment using Python 3.7.

```
python3.7 -m venv fenv
source fenv/bin/activate
```

2. Run the following command `python --version` and `python -m pip --version` to check that your Python 3.7 (and its accompanying pip package manager) is set up correctly.


3. Navigate to the cloned repository: `cd Fairify/` and install required packages:

```
python -m pip install -r requirements.txt
```

### Run the Fairify tool

Navigate to the src directory `cd src/` with command line. To run verification on all the models, run the following command. Here, `dataset` is any of the following: `AC`, `GC`, `BM`, or `compas`. All the models in one dataset are run with the following command:

```
./fairify.sh <dataset>
```

**Example:** To run the verification for all the 8 models in German Credit, run `./fairify.sh GC`. Each model is configured to be run for 30 minutes. The above command automatically runs for all the models in the corresponding dataset. The raw results will be generated in the `res` directory inside each dataset directory. All the results from our experiments are included in `csv` files.
