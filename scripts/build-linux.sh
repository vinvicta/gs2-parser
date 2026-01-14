#!/bin/bash
# GS2 Parser Build Script for Linux
# Usage: ./build-linux.sh [architecture]
# architectures: x86_64 (default), i686, aarch64, armv7l

set -e

ARCH=${1:-x86_64}
VERSION="1.5.0"
BUILD_DIR="build-release-linux-${ARCH}"
INSTALL_DIR="gs2parser-${VERSION}-linux-${ARCH}"

echo "Building GS2 Parser for Linux ${ARCH}..."

# Install dependencies if needed
if command -v apt-get &> /dev/null; then
    echo "Installing dependencies via apt..."
    sudo apt-get update
    sudo apt-get install -y cmake g++ bison flex
elif command -v yum &> /dev/null; then
    echo "Installing dependencies via yum..."
    sudo yum install -y cmake gcc-c++ bison flex
fi

# Set up cross-compilation if needed
case $ARCH in
    x86_64)
        CMAKE_CMD="cmake"
        ;;
    i686)
        CMAKE_CMD="cmake -DCMAKE_C_COMPILER=i686-linux-gnu-gcc -DCMAKE_CXX_COMPILER=i686-linux-gnu-g++"
        sudo apt-get install -y g++-multilib bison-multilib flex-multilib || true
        ;;
    aarch64)
        CMAKE_CMD="cmake -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
        sudo apt-get install -y g++-aarch64-linux-gnu bison flex || true
        ;;
    armv7l)
        CMAKE_CMD="cmake -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++"
        sudo apt-get install -y g++-arm-linux-gnueabihf bison flex || true
        ;;
    *)
        echo "Unknown architecture: $ARCH"
        exit 1
        ;;
esac

# Clean and create build directory
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure and build
echo "Configuring..."
$CMAKE_CMD -DCMAKE_BUILD_TYPE=Release ..

echo "Building..."
make -j$(nproc)

# Create release package
mkdir -p "$INSTALL_DIR"
cp bin/gs2test "$INSTALL_DIR/"
strip "$INSTALL_DIR/gs2test"

# Create README
cat > "$INSTALL_DIR/README.txt" << 'EOF'
GS2 Script Compiler/Disassembler
================================

Compile GS2 source to bytecode:
  ./gs2test script.gs2

Disassemble bytecode to readable format:
  ./gs2test script.gs2bc -d

For more options:
  ./gs2test --help

Documentation: https://github.com/vinvicta/gs2-parser
License: MIT
EOF

# Create tarball
tar -czf "gs2parser-${VERSION}-linux-${ARCH}.tar.gz" "$INSTALL_DIR"
sha256sum "gs2parser-${VERSION}-linux-${ARCH}.tar.gz" > "gs2parser-${VERSION}-linux-${ARCH}.tar.gz.sha256"

echo ""
echo "Build complete!"
echo "Package: gs2parser-${VERSION}-linux-${ARCH}.tar.gz"
echo "SHA256: $(cat gs2parser-${VERSION}-linux-${ARCH}.tar.gz.sha256)"
echo "Location: $(pwd)"
