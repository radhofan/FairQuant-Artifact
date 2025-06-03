#!/usr/bin/env bash

# Set up paths
export WORK_DIR="$PWD"
export OPENBLAS_SRC="$WORK_DIR/OpenBLAS-0.3.6"
export OPENBLAS_INSTALL="$WORK_DIR/OpenBLAS"
export FAIRQUANT_DIR="$WORK_DIR/FairQuant-Artifact"

# Clean up any previous failed OpenBLAS builds
rm -rf "$OPENBLAS_SRC" "$OPENBLAS_INSTALL" OpenBLAS-0.3.6.tar.gz

# Create install dir
mkdir -p "$OPENBLAS_INSTALL"

# Download OpenBLAS
echo "Downloading OpenBLAS..."
wget -O OpenBLAS-0.3.6.tar.gz \
  https://github.com/OpenMathLib/OpenBLAS/releases/download/v0.3.6/OpenBLAS-0.3.6.tar.gz 

# Extract OpenBLAS
echo "Extracting OpenBLAS..."
tar -xzf OpenBLAS-0.3.6.tar.gz

# Build OpenBLAS with safe settings
cd "$OPENBLAS_SRC" || exit 1

echo "Building OpenBLAS (generic target to avoid CPU-specific issues)..."
make clean
make -j$(nproc) USE_THREAD=0 TARGET=GENERIC_64

# If GENERIC_64 fails, try PRESCOTT or GENERIC_32
if [ $? -ne 0 ]; then
  echo "Build failed with TARGET=GENERIC_64, trying TARGET=PRESCOTT..."
  make clean
  make -j$(nproc) USE_THREAD=0 TARGET=PRESCOTT
fi

if [ $? -ne 0 ]; then
  echo "Build failed with TARGET=PRESCOTT, trying TARGET=GENERIC_32..."
  make clean
  make -j$(nproc) USE_THREAD=0 TARGET=GENERIC_32
fi

# Install OpenBLAS
echo "Installing OpenBLAS to $OPENBLAS_INSTALL..."
make PREFIX="$OPENBLAS_INSTALL" install

# Set environment variables for FairQuant
export C_INCLUDE_PATH="$OPENBLAS_INSTALL/include:$C_INCLUDE_PATH"
export LIBRARY_PATH="$OPENBLAS_INSTALL/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$OPENBLAS_INSTALL/lib:$LD_LIBRARY_PATH"

# Compile FairQuant (we assume FairQuant-Artifact is already there)
echo "Compiling FairQuant..."
cd "$FAIRQUANT_DIR/FairQuant" || exit 1
make clean
make all

# Run example
echo "Running example: adult dataset with 'sex' attribute..."
cd "$FAIRQUANT_DIR" || exit 1
./FairQuant/adult.sh sex