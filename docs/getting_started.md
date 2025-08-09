# Getting started

## 1. Installation

The compiler has to be installed from source.

1. Install [CMake](https://cmake.org/download/) (the minimum required version is 3.16)
2. Install a valid [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#id9) (I used [Ninja](https://ninja-build.org/))
3. Install [LLVM version 21 and Clang++](https://llvm.org/docs/GettingStarted.html#getting-the-source-code-and-building-llvm)
```bash
git clone --depth 1 https://github.com/llvm/llvm-project.git
cd llvm-project
cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" # Only use this last option if you don't already have a working installation of clang++
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
6. Clone this repository and initialise the submodules
```bash
git clone --recurse-submodules https://github.com/Achat-Dev/Kepler.git
cd Kepler
git submodule update --init --recursive
```
7. Build the project
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
cmake --build .
```

The result of this is the `kepler` executable.

> *(Disclaimer: the installation process as well as the given commands have only been tested on Ubuntu 24.4.2 LTS and Fedora Workstation 42)*

## 2. Usage

As of now, `kepler` only supports compiling a single `.kpl` file into an executable.

```bash
kepler -i <input.kpl> -o <output>
```

Additional `C++` or object files can be passed to the compiler using the `-a` option.

```bash
kepler -i <input.kpl> -o <output> -a foo.cpp,bar.cpp,baz.o
```

The `-v` option enables verbose logging, which outputs the generated LLVM IR.

Complete overview of the options (same as using `kepler -h`):

```bash
kepler [OPTION...]

-i, --input arg       The .kpl input files
-o, --output arg      The output file
-a, --additional arg  Additional C++ or object files, separated by ','
-v, --verbose         Enable verbose logging
-h, --help            Print help
```

> [!note]
> The order of the options doesn't matter, however, if `-h` is used, only the help is output and the program then terminates.

## 3. Language overview

### 3.1 Types

The following table displays the currently supported types.

| Name | Meaning | Additional notes |
| :- | :- | :- |
| `void` | Indicates a function with no return type | Can only be used as the return type of a function |
| `tmap` | A 48-bit struct containing each of the following types, as well as a flag for each field indicating whether a value has been assigned to that field | This is a value type |
| `bool` | A 1-bit unsigned integer that represents either `true` or `false` | Only the internal value used for operations is a 1-bit unsigned integer. An 8-bit unsigned integer is used in memory. |
| `string` | A pointer to an array of 8-bit signed integers | Strings are null terminated. String literals are stored as global constants, while all other strings, such as strings created through concatenation, are dynamically allocated on the heap. |
| `i8` | An 8-bit signed integer | |
| `i16` | A 16-bit signed integer | |
| `i32` | A 32-bit signed integer | |
| `i64` | A 64-bit signed integer | |
| `f32` | A 32-bit floating point value | Uses IEEE 754 semantics |
| `f64` | A 64-bit floating point value | Uses IEEE 754 semantics |

> [!note]
> The language doesn't support unsigned integers.

#### 3.1.1 Casting

Types can be casted by using the `type constructor`:

```
i32 foo = i32(4.2)  # casting a literal

f32 x = 1.0
foo = i32(x)        # casting a the value of a variable

foo = i32(bar())    # casting the return value of a function
```

Casting is the method used to access the fields of a `tmap`:

```
tmap x = foo()

i32 y = i32(x)
bool z = bool(x)
```

If no value is assigned to the field being accessed, a runtime error is thrown and the program terminates.

##### 3.1.1.1 Casting matrix

The following matrix displays what types can be casted to what types (rows are the type of the value to cast, columns are the target type of the cast):

| | `void` | `tmap` | `bool` | `string` | `i8` | `i16` | `i32` | `i64` | `f32` | `f64` |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| `void` | | | | | | | | | | |
| `tmap` | | | x | x | x | x | x | x | x | x |
| `bool` | | | | x | | | | | | |
| `string` | | | | | | | | | | |
| `i8` | | | x | x | | x | x | x | x | x |
| `i16` | | | x | x | x | | x | x | x | x |
| `i32` | | | x | x | x | x | | x | x | x |
| `i64` | | | x | x | x | x | x | | x | x |
| `f32` | | | x | x | x | x | x | x | | x |
| `f64` | | | x | x | x | x | x | x | x | |

> [!note]
> Trying to cast to the same type (e. g.: casting from an i32 to an i32) will result in a compile warning and the cast will be discarded

##### 3.1.1.2 Implicit casting

There is no implicit casting, except for the condition of an `if` expression.
Mismatching types will always result in a `type mismatch` compile error (even if the types are of the same type category, e. g.: `i16` and `i32`).

#### 3.1.2 Implicit type conversion

Some literals implicitly choose their type when used:

```
# floating point variable, integer literal -> the integer literal is implicitly converted to a floating point value
f32 x = 0
```

For more information and a detailed overview which types can implicitely convert to chich types refer to the [`TargetTypeStack`](./technical_documentation.md#422-determining-target-types-during-code-generation).

### 3.2 Variables

#### 3.2.1 Variable definition

Variables are defined after the following pattern:

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

##### 3.2.1.1 `tmap`

`tmaps` do not need to be defined in any special way.
By simply assigning a value to a `tmap`, this value is written to the field of the corresponding type within the `tmap`.

```
tmap x = false  # Creates a variable of type `tmap` and assigns 'false' to the bool field of 'x'
x = "Hello"     # Assigns 'Hello' to the string field of 'x'
x = 1337        # Assigns '1337' to the i32 field of 'x'
```

The fields of a `tmap` are not initialised by default.
As already mentioned, this leads to a runtime error when trying to access a field that hasn't been assigned a value yet.

#### 3.2.2 Variable uniqueness

Variables have to be unique within the scope they are defined in.

```
f32 x = 0
x = 1         # Ok
f32 x = -1    # Compile error
```

### 3.3 Control flow

#### 3.3.1 `if`

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

#### 3.3.2 `for`

##### 3.3.2.1 Indexed `for` loops

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

### 3.4 Functions

#### 3.4.1 Function definition

Functions are defined after the following pattern:

```
<return_type> <name>(<arguments>)
  <function_body...>
end
```

`<arguments>` are defined after the pattern `<arg_type> <arg_name>` and are separated by a comma (there can be 0-n arguments).

> [!note]
> A function signature (`<return_type> <name>(<arguments>)`) is called a `prototype`

#### 3.4.2 Function call

Functions are called by using brackets after the function name:

```
void foo()        # define a function without arguments and no return type
  ...
end

i32 bar(i32 a)    # define a function with an argument and a return type
  return a * a
end

foo()             # call foo
i32 x = bar(1)    # call bar
```

#### 3.4.3 `extern` functions

Prototypes marked with the `extern` keyword are raw prototypes with a function body that use external linkage to retrieve the function body.

```
extern <return_type> <name>(<arguments>)
```

#### 3.4.4 Return types and values

In general, a value of the specified return type has to be returned via the `return` keyword.
If a literal is returned, it tries to convert to the return type of the function.

### 3.5 Available runtime functions

The runtime provides the following functions.

| Function signature | Description |
| :- | :- |
| `print(string message)` | Prints `message` to the standard output with a newline character inserted after it |
| `pause()` | Pauses the application until an input is read |

### 3.6 Operators

<table>
  <tr>
    <th rowspan="2" style="text-align: left">Operator</th>
    <th rowspan="2" style="text-align: left">Usage example</th>
    <th rowspan="2" style="text-align: left">Usage notes</th>
    <th colspan="10">Supported types</th>
  </tr>
  <tr>
    <th>void</th>
    <th>tmap</th>
    <th>bool</th>
    <th>string</th>
    <th>i8</th>
    <th>i16</th>
    <th>i32</th>
    <th>i64</th>
    <th>f32</th>
    <th>f64</th>
  </tr>
  <tr>
    <td style="text-align: center">=</td>
    <td>&lt;variable_name&gt; = &lt;value&gt;</td>
    <td>Assigns &lt;value&gt; to the variable with &lt;variable_name&gt; and returns &lt;value&gt;</td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">+</td>
    <td>&lt;value_a&gt; + &lt;value_b&gt;</td>
    <td>Adds &lt;value_a&gt; and &lt;value_b&gt; together and returns the result</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">-</td>
    <td>&lt;value_a&gt; - &lt;value_b&gt;</td>
    <td>Subtracts &lt;value_b&gt; from &lt;value_a&gt; and returns the result</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">*</td>
    <td>&lt;value_a&gt; * &lt;value_b&gt;</td>
    <td>Multiplies &lt;value_a&gt; with &lt;value_b&gt; and returns the result</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">/</td>
    <td>&lt;value_a&gt; / &lt;value_b&gt;</td>
    <td>Divides &lt;value_a&gt; by &lt;value_b&gt; and returns the result</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">&lt;</td>
    <td>&lt;value_a&gt; &lt; &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is less than &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">&gt;</td>
    <td>&lt;value_a&gt; &gt; &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is greater than &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">==</td>
    <td>&lt;value_a&gt; == &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is equal to &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">!=</td>
    <td>&lt;value_a&gt; != &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is not equal to &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">&lt;=</td>
    <td>&lt;value_a&gt; &lt;= &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is less than or equal to &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
  <tr>
    <td style="text-align: center">&gt;</td>
    <td>&lt;value_a&gt; &gt;= &lt;value_b&gt;</td>
    <td>Evaluates to true, if &lt;value_a&gt; is greater than or equal to &lt;value_b&gt;, false otherwise</td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center"></td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
    <td style="text-align: center">x</td>
  </tr>
</table>

> [!note]
> The following applies to all operators listed here: the values / variables used must be of the same type.
