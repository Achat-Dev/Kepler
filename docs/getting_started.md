# Getting started

## 1. Installation

The compiler has to be installed from source.

1. Install [CMake](https://cmake.org/download/) (the minimum required version is 3.16)
2. Install [LLVM version 21 and Clang++](https://github.com/llvm/llvm-project/releases)
3. Clone this repository
```bash
git clone https://github.com/Achat-Dev/Kepler.git
cd Kepler
```
4. (Optional) Enable or disable assertions: Go to `src/utils/assert.hpp` and comment line 49 `#define KPL_NO_ASSERT` to disable them (**assertions are enabled by default**)
5. Build the project
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```

The result of this is the `kepler` executable.

> *(Disclaimer: the installation process has only been tested on various Linux systems)*

## 2. Usage

As of now, `kepler` only supports compiling a single `.kpl` file into an executable.

```bash
kepler -i <input.kpl> -o <output>
```

Additional `C` or object files can be passed to the compiler using the `-a` option.
CAUTION: The inputs for this option are not sanitized and passed directly to a shell command as is (which means that shell injections are possible and e. g. `foo.c; rm -rf \` will be executed).

```bash
kepler -i <input.kpl> -o <output> -a foo.c,bar.c,baz.o
```

Overview of the most important options (doesn't contain all information, execute `kepler -h` for all information):

```bash
kepler [OPTION...]

-i, --input arg               The .kpl input file
-o, --output arg              The output file
-a, --additional-files arg    Additional .c or .o files, separated by ','
-O, --optimization-level arg  The optimization level to use
-v, --version                 Print the compiler version
-h, --help                    Print help
```

> [!note]
> The order of the options doesn't matter

## 3. Hello, World!

Create a file called `main.kpl`, declare `printf` as an extern and call it inside the main function (the compiler links the final executable against `libc`, so we'll be using the `printf` function from there).

```
extern void printf(string s, ...)

i32 main()
  printf("Hello, World!\n")
  return 0
end
```

Then compile it into an executable...

```bash
kepler -i main.kpl -o main
```

...and run it.

```bash
./main
```
