#!/usr/bin/env bash

# Download make and gcc
sudo apt update
sudo apt install -y build-essential

# Download the tar file
wget https://github.com/OpenMathLib/OpenBLAS/releases/download/v0.3.6/OpenBLAS-0.3.6.tar.gz
tar -xzf OpenBLAS-0.3.6.tar.gz

# Set up installation path
export INSTALL_PREFIX=$HOME/OpenBLAS # or wherever you want to install OpenBLAS
mkdir $INSTALL_PREFIX

# Install
cd OpenBLAS-0.3.6
make
make PREFIX=$INSTALL_PREFIX install

# Check that OpenBLAS has been installed correctly
ls $INSTALL_PREFIX/include # you should see files such as cblas.h
ls $INSTALL_PREFIX/lib # you should see files such as libopenblas.so

export INSTALL_PREFIX=$HOME/OpenBLAS # or wherever you have installed OpenBLAS
export LIBRARY_PATH=$LIBRARY_PATH:$INSTALL_PREFIX/lib
export C_INCLUDE_PATH=$LD_LIBRARY_PATH:$INSTALL_PREFIX/include
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSTALL_PREFIX/lib

make -C FairQuant-Artifact/FairQuant all

./FairQuant-Artifact/FairQuant/adult.sh sex # 'sex' (in paper)
# ./FairQuant-Artifact/FairQuant/bank.sh age # 'age' (in paper)
# ./FairQuant-Artifact/FairQuant/german.sh age # 'age' (in paper) or 'sex'
# ./FairQuant-Artifact/FairQuant/compas.sh race