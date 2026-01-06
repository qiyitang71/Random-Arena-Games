# Building Game Graph Gym {#building}

This page provides detailed instructions for building Game Graph Gym with various configurations.
Here, "libggg" refers to the library, "ggg" is the short name for the project as well as the c++ namespace stuff is defined in.

## Requirements

### System Requirements

- **C++20 compatible compiler** (GCC 10+, Clang 10+, MSVC 2019+)
- **[CMake](https://cmake.org/) 3.15 or later**
- **[Boost Libraries](https://www.boost.org/) 1.70 or later**. Specifically, these components:
    - [`graph`](https://www.boost.org/doc/libs/release/libs/graph/) - Required for core graph data structures and algorithms
    - [`program_options`](https://www.boost.org/doc/libs/release/libs/program_options/) - Required for standardized solver CLI interface via `GGG_GAME_SOLVER_MAIN` macro
    - [`filesystem`](https://www.boost.org/doc/libs/release/libs/filesystem/) and [`system`](https://www.boost.org/doc/libs/release/libs/system/) - Optional, needed for command-line tools (build options `-DBUILD_TESTING=ON` or `-DBUILD_ALL_SOLVERS=ON`)
    - [`unit_test_framework`](https://www.boost.org/doc/libs/release/libs/test/) - Optional, needed for unit tests (build option `-DBUILD_TESTING=ON`)

### Platform-Specific Setup

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake \
    libboost-graph-dev libboost-program-options-dev \
    libboost-filesystem-dev libboost-system-dev libboost-test-dev
```

**Fedora/RHEL:**

```bash
sudo dnf install gcc-c++ cmake \
    boost-graph-devel boost-program-options-devel \
    boost-filesystem-devel boost-system-devel boost-test-devel
```

**macOS:**

```bash
brew install cmake boost
# Note: Homebrew's boost formula includes all components by default
```

**Windows (vcpkg):**

```bash
vcpkg install boost-graph boost-program-options boost-filesystem boost-system boost-test
```

## Build Configuration

### Basic Build

```bash
git clone https://github.com/pazz/ggg.git
cd ggg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build Options

Game Graph Gym provides several CMake options to customize the build:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_ALL_SOLVERS` | OFF | Build all included game solvers |
| `BUILD_TOOLS` | OFF | Build command-line benchmarking tools |
| `BUILD_TESTING` | OFF | Build unit tests |
| `CMAKE_BUILD_TYPE` | None | Build type (Debug/Release/RelWithDebInfo) |

### Complete Build

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_ALL_SOLVERS=ON \
    -DBUILD_TOOLS=ON \
    -DBUILD_TESTING=ON
    
cmake --build build -j$(nproc)
```

### Custom Boost Installation

If Boost is installed in a non-standard location:

```bash
cmake -S . -B build \
    -DBOOST_ROOT=/path/to/boost \
    -DBoost_NO_SYSTEM_PATHS=ON
```

## Installation

You can optionally install the library system-wide:

```bash
cmake --build build --target install
```

Or to a custom prefix:

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/custom/path
cmake --build build --target install
```

As `libggg` is header-only, this will only install header files.
You can then `#include <libggg/...>` directly.

## Using Game Graph Gym in Your Project

### CMake Integration

Add Game Graph Gym to your CMake project:

```cmake
find_package(GameGraphGym REQUIRED)
target_link_libraries(your_target PRIVATE GameGraphGym::ggg)
```

### Manual Integration

If building without installation:

```cmake
# Find Boost library. Libggg requires graph and program_options components 
# (also add `system`, `filesystem` for tools, and `unit_test_framework` for tests)
find_package(Boost REQUIRED COMPONENTS graph program_options)

# Add Game Graph Gym
add_subdirectory(path/to/libggg)
target_link_libraries(your_target PRIVATE ggg Boost::graph Boost::program_options)
target_compile_features(your_target PRIVATE cxx_std_20)
```

## Build Artifacts

After building, you'll find:

- **Headers**: `include/libggg/` (all functionality is header-only; no static or shared library is produced)
- **Solvers**: `build/solvers/game_type/solver_name/`
- **Tools**: `build/bin/`
- **Tests**: `build/test/`

## Troubleshooting

### Common Issues

**Boost not found:**

```bash
CMAKE_ERROR: Could not find Boost
```

Solution: Install Boost development packages or set `BOOST_ROOT`

**C++20 not supported:**

```bash
error: 'auto' not allowed in non-static class member
```

Solution: Use a newer compiler or set `CMAKE_CXX_STANDARD=20`

**Link errors with Boost:**

```bash
undefined reference to boost::graph
```

Solution: Ensure CMake policy is set: `cmake_policy(SET CMP0167 NEW)`

### Performance Builds

For maximum performance:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG -march=native"
```

### Debug Builds

For debugging with full symbols:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-g -O0"
```
