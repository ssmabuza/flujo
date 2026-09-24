#!/bin/bash

# Post-create setup script for Flujo devcontainer
echo "Setting up Flujo development environment..."

# Set environment variables for development
export MPI_DIR=/usr
export SOFTWARE=/opt/software  
export TRILINOS_HOME=${SOFTWARE}/trilinos/Trilinos
export INSTALL_DIR=${SOFTWARE}/trilinos
export CMAKE_PREFIX_PATH=${INSTALL_DIR}:${CMAKE_PREFIX_PATH}
export LD_LIBRARY_PATH=${INSTALL_DIR}/lib:${LD_LIBRARY_PATH}

# Add environment variables to .bashrc for persistence
cat >> ~/.bashrc << 'EOF'

# Flujo development environment
export MPI_DIR=/usr
export SOFTWARE=/opt/software
export TRILINOS_HOME=${SOFTWARE}/trilinos/Trilinos
export INSTALL_DIR=${SOFTWARE}/trilinos
export CMAKE_PREFIX_PATH=${INSTALL_DIR}:${CMAKE_PREFIX_PATH}
export LD_LIBRARY_PATH=${INSTALL_DIR}/lib:${LD_LIBRARY_PATH}
EOF

# Verify Trilinos installation
if [ -f "${INSTALL_DIR}/lib/cmake/Trilinos/TrilinosConfig.cmake" ]; then
    echo "✅ Trilinos installation found"
else
    echo "❌ Trilinos installation not found"
fi

# Verify MPI
if command -v mpirun &> /dev/null; then
    echo "✅ MPI found: $(mpirun --version | head -n1)"
else
    echo "❌ MPI not found"
fi

# Verify CMake
if command -v cmake &> /dev/null; then
    echo "✅ CMake found: $(cmake --version | head -n1)"
else
    echo "❌ CMake not found"
fi

# Create build directory if it doesn't exist
mkdir -p build

echo "🚀 Flujo development environment ready!"
echo ""
echo "To build Flujo:"
echo "  cmake --preset debug"
echo "  cmake --build build/debug"
echo ""
echo "Environment variables are set in ~/.bashrc"