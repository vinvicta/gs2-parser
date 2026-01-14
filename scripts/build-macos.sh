#!/bin/bash
# GS2 Parser Build Script for macOS
# Builds universal binary supporting x86_64 and arm64 (Apple Silicon)

set -e

VERSION="1.5.0"
BUILD_DIR="build-release-macos"
INSTALL_DIR="gs2parser-${VERSION}-macos-universal"

echo "Building GS2 Parser for macOS (Universal Binary)..."

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "Error: This script must be run on macOS"
    exit 1
fi

# Install dependencies if needed
if ! command -v cmake &> /dev/null; then
    echo "Installing CMake via Homebrew..."
    if ! command -v brew &> /dev/null; then
        echo "Error: Homebrew not found. Please install from https://brew.sh"
        exit 1
    fi
    brew install cmake
fi

if ! command -v bison &> /dev/null; then
    echo "Installing Bison via Homebrew..."
    brew install bison
fi

if ! command -v flex &> /dev/null; then
    echo "Installing Flex via Homebrew..."
    brew install flex
fi

# Detect architecture
ARCH=$(uname -m)
echo "Current architecture: $ARCH"

# Set up build directories
BUILD_X64="build-macos-x86_64"
BUILD_ARM="build-macos-arm64"

# Build for current architecture
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring..."
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="$ARCH" ..

echo "Building..."
make -j$(sysctl -n hw.ncpu)

# Create release package
mkdir -p "$INSTALL_DIR"
cp bin/gs2test "$INSTALL_DIR/"
strip "$INSTALL_DIR/gs2test"

# Create README
cat > "$INSTALL_DIR/README.txt" << 'EOF'
GS2 Script Compiler/Disassembler for macOS
==========================================

Compile GS2 source to bytecode:
  ./gs2test script.gs2

Disassemble bytecode to readable format:
  ./gs2test script.gs2bc -d

For more options:
  ./gs2test --help

Documentation: https://github.com/vinvicta/gs2-parser
License: MIT

Note: This is a single-architecture binary for $ARCH.
To build a universal binary supporting both x86_64 and arm64,
build separately on each architecture and use lipo to combine.
EOF

# Replace $ARCH in README
sed -i '' "s/\$ARCH/$ARCH/" "$INSTALL_DIR/README.txt"

# Create tarball
tar -czf "gs2parser-${VERSION}-macos-${ARCH}.tar.gz" "$INSTALL_DIR"
shasum -a 256 "gs2parser-${VERSION}-macos-${ARCH}.tar.gz" > "gs2parser-${VERSION}-macos-${ARCH}.tar.gz.sha256"

echo ""
echo "Build complete!"
echo "Package: gs2parser-${VERSION}-macos-${ARCH}.tar.gz"
echo "SHA256: $(cat gs2parser-${VERSION}-macos-${ARCH}.tar.gz.sha256)"
echo "Location: $(pwd)"

# Instructions for universal binary
if [ "$ARCH" = "arm64" ]; then
    echo ""
    echo "To create a universal binary:"
    echo "1. Build on x86_64 Mac: ./build-macos.sh"
    echo "2. Build on arm64 Mac: ./build-macos.sh"
    echo "3. Combine using: lipo -create build-macos-x86_64/gs2test build-macos-arm64/gs2test -output gs2test-universal"
fi
