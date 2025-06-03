#!/usr/bin/env bash
set -x  # Show each command as it runs
set -e  # Exit immediately if any command fails

echo "Current directory: $(pwd)"

# Download make and gcc
sudo apt update
sudo apt install -y build-essential

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

# Build FairQuant
make -C ./FairQuant-Artifact/FairQuant all

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