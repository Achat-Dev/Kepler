# Kepler programming language

## Installation guide

1. Install [CMake](https://cmake.org/download/) (the minimum required version is 3.8)
```bash
sudo apt install cmake
```
2. Install a valid [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#id9) (I used [Ninja](https://ninja-build.org/))
```bash
sudo apt install ninja-build
```
3. Install [LLVM](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
```bash
git clone --depth 1 https://github.com/llvm/llvm-project.git
cd llvm-project
cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build . --target install
```
4. Clone this repository
```bash
git clone https://github.com/Achat-Dev/Kepler.git
```
5. Build the project
```bash
cd Kepler
cmake -S . -B build
cd build
cmake --build .
```

The result of this is the `kepler` executable.

> *(Disclaimer: the installation process as well as the given commands have only been tested on Ubuntu 24.4.2 LTS.)*
