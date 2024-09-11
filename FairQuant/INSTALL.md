# FairQuant Installation

Our tool is tested on Ubuntu 16.04. The only additional installation needed is for OpenBLAS library.

You can follow the [OpenBLAS's Installation Guide](https://github.com/OpenMathLib/OpenBLAS/wiki/Installation-Guide) or run the following commands:

```
wget https://github.com/xianyi/OpenBLAS/archive/v0.3.6.tar.gz
tar -xzf v0.3.6.tar.gz
cd OpenBLAS-0.3.6
make
export INSTALL_PREFIX=$HOME/OpenBLAS
mkdir $INSTALL_PREFIX
make PREFIX=$INSTALL_PREFIX install
```

Note that the same INSTALL_PREFIX should be used in the makefile.