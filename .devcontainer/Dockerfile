FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Create vscode user
RUN groupadd --gid 1000 vscode \
    && useradd --uid 1000 --gid vscode --shell /bin/bash --create-home vscode

# Install base packages
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    clang-format \
    clang-tools \
    clang-tidy \
    clangd \
    git \
    wget \
    curl \
    vim \
    sudo \
    gdb \
    valgrind \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install newer CMake (3.28+) from Kitware
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | sudo tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    apt-get install -y cmake && \
    rm -rf /var/lib/apt/lists/*

# Install MPI
RUN apt-get update && apt-get install -y \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*

# Install scientific computing dependencies
RUN apt-get update && apt-get install -y \
    libhdf5-openmpi-dev \
    libnetcdf-dev \
    libnetcdf-c++4-dev \
    liblapack-dev \
    libblas-dev \
    libboost-all-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set up environment variables
ENV MPI_DIR=/usr
ENV SOFTWARE=/opt/software
ENV TRILINOS_HOME=${SOFTWARE}/trilinos/Trilinos
ENV INSTALL_DIR=${SOFTWARE}/trilinos

# Create directories
RUN mkdir -p ${SOFTWARE} && \
    chown -R vscode:vscode ${SOFTWARE}

# Switch to vscode user for Trilinos build
USER vscode

# Clone and build Trilinos
WORKDIR ${SOFTWARE}
RUN git clone https://github.com/trilinos/Trilinos.git trilinos/Trilinos

# Copy and run Trilinos build script
COPY --chown=vscode:vscode .devcontainer/build-trilinos.sh ${SOFTWARE}/
RUN chmod +x ${SOFTWARE}/build-trilinos.sh

WORKDIR ${SOFTWARE}/trilinos/Trilinos
RUN ${SOFTWARE}/build-trilinos.sh

# Install Trilinos
RUN cd build && make install -j$(nproc)

# Set up final environment
ENV CMAKE_PREFIX_PATH=${INSTALL_DIR}:${CMAKE_PREFIX_PATH}
ENV LD_LIBRARY_PATH=${INSTALL_DIR}/lib:${LD_LIBRARY_PATH}

# Give vscode sudo access
USER root
RUN echo 'vscode ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER vscode
WORKDIR /workspaces/flujo