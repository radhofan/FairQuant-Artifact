#!/usr/bin/env bash

# === Set up directories ===
export WORK_DIR="$PWD"
export LOCAL_INSTALL="$WORK_DIR/local"
export OPENBLAS_SRC="$WORK_DIR/OpenBLAS-0.3.6"
export OPENBLAS_INSTALL="$WORK_DIR/OpenBLAS"
export FAIRQUANT_DIR="$WORK_DIR/FairQuant-Artifact/FairQuant"

# Add local binaries and libraries to environment
export PATH="$LOCAL_INSTALL/bin:$PATH"
export C_INCLUDE_PATH="$LOCAL_INSTALL/include:$OPENBLAS_INSTALL/include:$C_INCLUDE_PATH"
export LIBRARY_PATH="$LOCAL_INSTALL/lib:$LOCAL_INSTALL/lib64:$OPENBLAS_INSTALL/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$LOCAL_INSTALL/lib:$LOCAL_INSTALL/lib64:$OPENBLAS_INSTALL/lib:$LD_LIBRARY_PATH"

mkdir -p "$LOCAL_INSTALL" "$OPENBLAS_INSTALL" "$FAIRQUANT_DIR"

# === Check for make and gcc, install from source if missing ===

# ---- Install Make ----
if ! command -v make &>/dev/null; then
  echo "Installing make..."
  wget -O "$WORK_DIR/make.tar.gz" https://ftp.gnu.org/gnu/make/make-4.3.tar.gz 
  tar -xzf "$WORK_DIR/make.tar.gz" -C "$WORK_DIR"
  MAKE_DIR="$WORK_DIR/make-4.3"
  mkdir -p "$MAKE_DIR/build"
  cd "$MAKE_DIR" && ./configure --prefix="$LOCAL_INSTALL" && make && make install
  export PATH="$LOCAL_INSTALL/bin:$PATH"
fi

# ---- Install GCC ----
if ! command -v gcc &>/dev/null; then
  echo "Installing GMP..."
  wget -O "$WORK_DIR/gmp.tar.xz" https://ftp.gnu.org/gnu/gmp/gmp-6.2.1.tar.xz 
  tar -xf "$WORK_DIR/gmp.tar.xz" -C "$WORK_DIR"
  GMP_DIR="$WORK_DIR/gmp-6.2.1"
  cd "$GMP_DIR" && ./configure --prefix="$LOCAL_INSTALL" && make && make install

  echo "Installing MPFR..."
  wget -O "$WORK_DIR/mpfr.tar.gz" https://www.mpfr.org/mpfr-current/mpfr-4.1.0.tar.gz 
  tar -xzf "$WORK_DIR/mpfr.tar.gz" -C "$WORK_DIR"
  MPFR_DIR="$WORK_DIR/mpfr-4.1.0"
  cd "$MPFR_DIR" && ./configure --prefix="$LOCAL_INSTALL" --with-gmp="$LOCAL_INSTALL" && make && make install

  echo "Installing MPC..."
  wget -O "$WORK_DIR/mpc.tar.gz" https://ftp.gnu.org/gnu/mpc/mpc-1.2.1.tar.gz 
  tar -xzf "$WORK_DIR/mpc.tar.gz" -C "$WORK_DIR"
  MPC_DIR="$WORK_DIR/mpc-1.2.1"
  cd "$MPC_DIR" && ./configure --prefix="$LOCAL_INSTALL" --with-gmp="$LOCAL_INSTALL" --with-mpfr="$LOCAL_INSTALL" && make && make install

  echo "Installing GCC..."
  wget -O "$WORK_DIR/gcc.tar.gz" https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.gz 
  tar -xzf "$WORK_DIR/gcc.tar.gz" -C "$WORK_DIR"
  GCC_DIR="$WORK_DIR/gcc-11.2.0"
  mkdir -p "$WORK_DIR/gcc-build"
  cd "$GCC_DIR" && ./contrib/download_prerequisites
  cd "$WORK_DIR/gcc-build" && \
    ../gcc-11.2.0/configure --prefix="$LOCAL_INSTALL" \
      --enable-languages=c,c++ \
      --disable-multilib \
      --with-gmp="$LOCAL_INSTALL" \
      --with-mpfr="$LOCAL_INSTALL" \
      --with-mpc="$LOCAL_INSTALL"
  make
  make install
  export PATH="$LOCAL_INSTALL/bin:$PATH"
fi

# === Download and build OpenBLAS ===
echo "Downloading and building OpenBLAS..."
wget -O "$WORK_DIR/openblas.tar.gz" https://github.com/OpenMathLib/OpenBLAS/releases/download/v0.3.6/OpenBLAS-0.3.6.tar.gz 
tar -xzf "$WORK_DIR/openblas.tar.gz" -C "$WORK_DIR"

cd "$OPENBLAS_SRC" && make USE_THREAD=0 PREFIX="$OPENBLAS_INSTALL" install

# Update env vars again
export C_INCLUDE_PATH="$OPENBLAS_INSTALL/include:$C_INCLUDE_PATH"
export LIBRARY_PATH="$OPENBLAS_INSTALL/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$OPENBLAS_INSTALL/lib:$LD_LIBRARY_PATH"

# === Build FairQuant ===
cd "$WORK_DIR" && \
  git clone https://github.com/yourusername/FairQuant-Artifact.git  || true

cd "$FAIRQUANT_DIR" && make clean && make all

# === Run FairQuant Example ===
cd "$WORK_DIR/FairQuant-Artifact" && \
  ./FairQuant/adult.sh sex