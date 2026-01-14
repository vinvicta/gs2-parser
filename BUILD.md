# Building GS2 Parser

This document provides detailed instructions for building the GS2 Parser/Disassembler on different platforms.

## Quick Start

### Linux
```bash
git clone https://github.com/vinvicta/gs2-parser.git --recursive
cd gs2-parser
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS
```bash
brew install cmake bison flex
git clone https://github.com/vinvicta/gs2-parser.git --recursive
cd gs2-parser
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Windows
```cmd
vcpkg install cmake bison flex --triplet x64-windows
git clone https://github.com/vinvicta/gs2-parser.git --recursive
cd gs2-parser
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ..
cmake --build . --config Release
```

## Automated Build Scripts

We provide automated build scripts for each platform:

### Linux Build Script
```bash
./scripts/build-linux.sh [architecture]
```

Supported architectures:
- `x86_64` (default)
- `i686` (32-bit)
- `aarch64` (ARM64)
- `armv7l` (ARM 32-bit)

### macOS Build Script
```bash
./scripts/build-macos.sh
```

Builds for the current architecture (x86_64 or arm64).

### Windows Build Script
```cmd
scripts\build-windows.bat
```

## Cross-Compilation

### Linux to ARM

Install cross-compilation tools:
```bash
# For ARM64 (aarch64)
sudo apt-get install g++-aarch64-linux-gnu

# For ARM32 (armv7l)
sudo apt-get install g++-arm-linux-gnueabihf
```

Build with cross-compiler:
```bash
mkdir build && cd build
CMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
CMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
cmake ..
make -j$(nproc)
```

### macOS Universal Binary

To create a universal binary supporting both x86_64 and arm64:

1. Build on an Intel Mac:
```bash
./scripts/build-macos.sh  # Creates gs2test-x86_64
```

2. Build on an Apple Silicon Mac:
```bash
./scripts/build-macos.sh  # Creates gs2test-arm64
```

3. Combine using `lipo`:
```bash
lipo -create gs2test-x86_64 gs2test-arm64 -output gs2test-universal
```

## Release Build Process

### Using GitHub Actions (Recommended)

When you push a tag like `v1.5.0`, the GitHub Actions workflow automatically builds binaries for:
- Linux x86_64
- macOS x86_64
- macOS ARM64 (Apple Silicon)
- Windows x64

The artifacts are uploaded to the release automatically.

### Manual Release Build

1. **Clean previous builds:**
```bash
rm -rf build-release-*
```

2. **Build for each platform:**
```bash
# Linux
./scripts/build-linux.sh x86_64

# macOS (run on macOS)
./scripts/build-macos.sh

# Windows (run on Windows)
scripts\build-windows.bat
```

3. **Collect packages:**
```bash
ls -lh build-release-*/gs2parser-*.tar.gz
ls -lh build-release-*/gs2parser-*.zip  # Windows
```

4. **Verify checksums:**
```bash
cat build-release-*/gs2parser-*.sha256
```

## Building with Different CMake Generators

### Ninja (Faster builds)

```bash
# Install Ninja
# Linux: sudo apt-get install ninja-build
# macOS: brew install ninja

# Build with Ninja
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### Xcode (macOS)

```bash
mkdir build && cd build
cmake -G Xcode ..
xcodebuild -configuration Release
```

### Visual Studio (Windows)

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## Build Options

### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Build with Symbols
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```

### Static Linking (Linux)
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_EXE_LINKER_FLAGS="-static" ..
make
```

Note: Static linking may cause issues with Flex/Bison libraries.

## Dependencies

### Required
- **CMake** (3.10+)
- **C++ Compiler** with C++17 support:
  - GCC 7+
  - Clang 5+
  - MSVC 2017+
- **Bison** (3.4+)
- **Flex** (2.6+)

### Optional
- **Ninja** - Faster builds
- **ccache** - Faster incremental builds

## Platform-Specific Notes

### Linux
- Most distributions provide all required dependencies
- On Ubuntu/Debian: `sudo apt-get install cmake g++ bison flex`
- On Fedora/RHEL: `sudo dnf install cmake gcc-c++ bison flex`
- On Arch: `sudo pacman -S cmake gcc bison flex`

### macOS
- Install Xcode Command Line Tools: `xcode-select --install`
- Install Homebrew from https://brew.sh
- Install dependencies: `brew install cmake bison flex`

Note: macOS may require you to set `BISON_EXECUTABLE` and `FLEX_EXECUTABLE` CMake variables if using Homebrew:

```bash
cmake -DBISON_EXECUTABLE=$(brew --prefix bison)/bin/bison \
      -DFLEX_EXECUTABLE=$(brew --prefix flex)/bin/flex \
      ..
```

### Windows
- Install vcpkg from https://vcpkg.io
- Bootstrap vcpkg: `.\vcpkg\bootstrap-vcpkg.bat`
- Install dependencies: `vcpkg install cmake bison flex --triplet x64-windows`
- Set `CMAKE_TOOLCHAIN_FILE` to vcpkg's toolchain file

## Troubleshooting

### "bison not found"

**Solution:** Install Bison and ensure it's in PATH.

**macOS:** `brew install bison`

**Linux:** `sudo apt-get install bison`

**CMake variable:** `-DBISON_EXECUTABLE=/path/to/bison`

### "flex not found"

**Solution:** Install Flex and ensure it's in PATH.

**macOS:** `brew install flex`

**Linux:** `sudo apt-get install flex`

**CMake variable:** `-DFLEX_EXECUTABLE=/path/to/flex`

### "C++17 required"

**Solution:** Ensure you're using a modern compiler.

**GCC:** Version 7 or later (`g++ --version`)

**Clang:** Version 5 or later (`clang++ --version`)

**MSVC:** Visual Studio 2017 or later

### Build fails on macOS with "library not loaded"

**Solution:** This is usually due to Homebrew installing in `/usr/local` on Intel Macs and `/opt/homebrew` on Apple Silicon. Ensure your PATH is set correctly:

```bash
# Intel Mac
export PATH="/usr/local/opt/bison/bin:/usr/local/opt/flex/bin:$PATH"

# Apple Silicon
export PATH="/opt/homebrew/opt/bison/bin:/opt/homebrew/opt/flex/bin:$PATH"
```

## Advanced: Building WebAssembly Version

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build
cd gs2-parser
mkdir build && cd build
emcmake cmake ..
make -j$(nproc)
```

This produces `gs2test.js` and `gs2test.wasm` for web usage.

## Packaging

The build scripts automatically create release packages with:
- Compiled binary (stripped)
- README with usage instructions
- SHA256 checksum

Package formats:
- **Linux**: `.tar.gz`
- **macOS**: `.tar.gz`
- **Windows**: `.zip`

## CI/CD

The project includes GitHub Actions workflows (`.github/workflows/release.yml`) that automatically build releases for all platforms when you push a version tag.

To trigger a release build:
```bash
git tag v1.5.0
git push origin v1.5.0
```

The workflow will build binaries and attach them to the GitHub release.

## Support

For build issues:
1. Check the [Troubleshooting](#troubleshooting) section
2. Search existing GitHub Issues
3. Create a new issue with:
   - Platform and OS version
   - Compiler version
   - Full CMake output
   - Error messages
