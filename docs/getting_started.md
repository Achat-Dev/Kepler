# Getting started

## 1. Installation

The compiler for Kepler has to be installed from source.

1. Install [CMake](https://cmake.org/download/) (the minimum required version is 3.8)
2. Install a valid [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#id9) (e. g. [Ninja](https://ninja-build.org/))
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
cmake --build . --target install
```

The result of this is the `kepler` executable.

## 2. Usage

> [!important]
> This section will change heavily as the development of the project is progressing

As of now, Kepler only supports the compilation of single files to object code.

```bash
kepler <input> <output>
```

## 3. Language overview

### Misc

- Only one statement per line is allowed
- Blocks are ended with the end statement

### 3.1 Variables

Variables are defined after the following scheme:

```
<type> <name> = <value>
```

Variables always have do be defined with an initial value.

```
f32 x = 0   # Ok
f32 y       # Compile error
```

Both variable definitions and variable assignments return the value assigned to the variable.

```
# Multiple variable definition
f32 x = f32 y = 0

# Multiple variable assignment
x = y = 42

# Variable definition in if
if (f32 z = foo())
  bar(z)
end
```

Variables have to be unique within the scope they are defined in.

```
f32 x = 0
x = 1         # Ok
f32 x = -1    # Compile error
```

### 3.2 Control flow

#### 3.2.1 `if`

```
if (<condition>)
  ...
end
```

When chaining `if`, `elseif` and `else`, the entire chain has to be terminated with the `end` keyword (not every individual branch).

```
if (<condition>)
  ...
elseif (<condition>)
  ...
else
  ...
end
```

#### 3.2.2 `for`

##### 3.2.2.1 Indexed `for` loops

A basic indexed for loops consists of a variable declaration followed by a colon and three expressions.
The first expression is the start value of the loop variable, the second is the stop value (exclusive) and the third is the step value which is added to the loop variable after each loop iteration.

```
for (<type> <name> : <start>, <stop>, <step>)
  ...
end
```

`<step>` can be omitted. If omitted, the step value will either be `1` or `-1`, depending on whether `<start>` is less than or greater than `<stop>`.

```
for (<type> <name> : <start>, <stop>)
  ...
end
```

If `<step>` is omitted, `<start>` can also be omitted to initialise the loop variable with a value of `0`.

```
for (<type> <name> : <stop>)
  ...
end
```

> [!note]
> Only integer and floating point types are allowed for the type of the loop variable.

### 3.3 Functions

### 3.4 Types

#### 3.4.1 Casting

### 3.5 Operators
