#!/usr/bin/env bash
set -x  # Show each command as it runs
set -e  # Exit immediately if any command fails

echo "Current directory: $(pwd)"

# Download make and gcc
sudo apt update
sudo apt install -y build-essential
sudo apt install csvtool
sudo apt install -y python3-swiftclient

echo "Verifying installation..."
gcc --version
make --version

# Download the tar file
wget https://github.com/OpenMathLib/OpenBLAS/releases/download/v0.3.6/OpenBLAS-0.3.6.tar.gz   
tar -xzf OpenBLAS-0.3.6.tar.gz

# Set up installation path
export INSTALL_PREFIX=$HOME/OpenBLAS
mkdir -p "$INSTALL_PREFIX"

# Install OpenBLAS
cd OpenBLAS-0.3.6
make
make PREFIX="$INSTALL_PREFIX" install

# Set environment variables
export LIBRARY_PATH="$INSTALL_PREFIX/lib:$LIBRARY_PATH"
export C_INCLUDE_PATH="$INSTALL_PREFIX/include:$C_INCLUDE_PATH"
export LD_LIBRARY_PATH="$INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"  

cd ..

echo "Current directory: $(pwd)"
ls -la FairQuant-Artifact/FairQuant || echo "Directory missing!"

# Build FairQuant
FAIRQUANT_DIR="$PWD/FairQuant-Artifact/FairQuant"
make -C "$FAIRQUANT_DIR" all

# Verify adult.sh exists and is executable
ADULT_SCRIPT="./FairQuant-Artifact/FairQuant/adult.sh"
if [ ! -f "$ADULT_SCRIPT" ]; then
    echo "ERROR: $ADULT_SCRIPT does not exist!" >&2
    exit 1
fi

if [ ! -x "$ADULT_SCRIPT" ]; then
    echo "ERROR: $ADULT_SCRIPT is not executable!" >&2
    chmod +x "$ADULT_SCRIPT"
fi

# Run adult.sh
echo "Running $ADULT_SCRIPT with argument 'sex'"
"$ADULT_SCRIPT" sex

source ~/openrc

bucket_name="bare_metal_experiment_pattern_data"  # Simple, static bucket name
file_to_upload="FairQuant-Artifact/FairQuant/counterexamples_forward.csv"

echo
echo "Uploading results to the object store container $bucket_name"
# Create the bucket if it doesn't exist
swift post $bucket_name

# Upload just the counterexamples.csv file
if [ -f "$file_to_upload" ]; then
    echo "Uploading $file_to_upload"
    swift upload "$bucket_name" "$file_to_upload" --object-name "counterexamples.csv"
else
    echo "ERROR: File $file_to_upload does not exist!" >&2
    exit 1
fi