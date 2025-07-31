# Kepler programming language

## Installation guide

1. Install [CMake](https://cmake.org/download/) (the minimum required version is 3.8)
2. Install a valid [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#id9) (I used [Ninja](https://ninja-build.org/))
3. Install [LLVM and Clang](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
```bash
git clone --depth 1 https://github.com/llvm/llvm-project.git
cd llvm-project
cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" # Only use this last option if you don't already have a working installation of clang
cd build
cmake --build . --target install
```
4. Install [bdwgc](https://github.com/bdwgc/bdwgc?tab=readme-ov-file#building-and-installing) (also known as `libgc`)
```bash
git clone https://github.com/bdwgc/bdwgc.git
cd bdwgc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build . --target install
```
5. Install [xxd](https://github.com/ckormanyos/xxd)
6. Clone this repository
```bash
git clone https://github.com/Achat-Dev/Kepler.git
```
7. Build the project
```bash
cd Kepler
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```

The result of this is the `kepler` executable.

> *(Disclaimer: the installation process as well as the given commands have only been tested on Ubuntu 24.4.2 LTS.)*
