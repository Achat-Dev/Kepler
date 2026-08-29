# Language Overview

This page serves as a quick reference for examples of the base language features.
The four most important things to keep in mind about `kepler` are:

1. Everything is explicitely typed
2. There are no semicolons
3. Control flow doesn't use curly braces. Instead, functions and statements are closed with the `end` keyword.
4. The language is pretty minimal: it doesn't have a custom runtime and is only linked against `libc` by default.

## 1. Functions

Functions are defined after the following pattern:

```
<return_type> <name>(<arguments>)
  ...
end
```

All functions use external linkage, which means that they can easily be called from e. g. `C` code by declaring them as `extern` functions there.

```
# lib.kpl
i32 add(i32 a, i32 b)
  return a + b
end

# main.c
#include <stdint.h>

extern int32_t add(in32_t a, int32_t b);

int main() {
  int32_t x = add(2, 2);
  return 0;
}
```

## 1.1 Extern functions

Functions without a body can be declared as `extern` inside of a `.kpl` file to call external functions.

```
# lib.c
#include <stdint.h>

int32_t add(in32_t a, int32_t b) {
  return a + b;
}

# main.kpl
extern i32 add(i32 a, i32 b)

i32 main()
  i32 x = add(2, 2)
  return 0
end
```

## 1.2 Variadic functions

A variadic function (a function that can take a variable number of arguments of arbitrary types) can be defined by using `...` as the last parameter.

```
extern void printf(string s, ...)
```

There is currently no way to access the variadic arguments inside of the function.
The feature is mostly there to be able to call some `C` functions like `printf`

```
# This is legal syntax and the function can be called, but there isn't really a point
void foo(i32x, ...)
  # No way to access the values of '...'
end
```

## 1.3 Function overloading

There currently is no function overloading, so once a function is defined, that name cannot be used for another function.

## 2. Variables

Variables are defined after the following pattern:

```
<type> <name> = <value>
```

Variables always have to be defined with an initial value.

```
f32 x = 0   # Ok
f32 y       # Compile error
```

Variables can only be defined inside of functions -- there are no global variables yet.

## 3. Control flow

### 3.1 If statement

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

### 3.2 For statement

A basic `for` loop consists of a variable declaration followed by a colon and three expressions.
The first expression is the start value (inclusive), the second is the stop value (exclusive) and the third is the step value which is added to the loop variable after each loop iteration.

```
for (<type> <name> : <start>, <stop>, <step>)
  ...
end

# Example: iterate from 0 to 10 with a step of 1
for (i32 i : 0, 10, 1)
  ...
end
```

`<step>` is optional. If omitted, the step value will either be `1` or `-1`, depending on whether `<start>` is less than or greater than `<stop>`.

```
for (<type> <name> : <start>, <stop>)
  ...
end

# Example: iterate from 0 to 10, step is implicitely 1
for (i32 i : 0, 10)
  ...
end

# Example: iterate from 10 to 0, step is implicitely -1
for (i32 i : 10, 0)
  ...
end
```

`<start>` is also optional and can be omitted if `<step>` is omitted to initialise the loop variable with a value of `0`.

```
for (<type> <name> : <stop>)
  ...
end

# Example: iterate from 0 to 10, start is implicitely 0, step is implicitely 1
for (i32 i : 10)
  ...
end
```

> [!note]
> Only integer types are allowed for the type of the loop variable.

## 4. Casting

Values can be casted by using the `type constructor`:

```
i32 foo = i32(4.2)  # casting a literal

f32 x = 1.0
foo = i32(x)        # casting the value of a variable

foo = i32(bar())    # casting the return value of a function
```

Trying to cast to the same type (e. g.: casting from an i32 to an i32) will result in a compile warning and the cast will be discarded.

Be aware that there is no implicit casting -- everything has to be casted explicitely.

```
i32 x = 0
i64 y = 0
i64 z = x + y       # Compile error, type mismatch
i64 z = i64(x) + y  # Ok
```

## 5. Type system

Currently, there are only basic builtin types and no user defined types.

### 5.1 Supported types

| Name | Meaning | Additional notes |
| :- | :- | :- |
| `void` | Indicates a function with no return type | Can only be used as the return type of a function |
| `bool` | A 1-bit unsigned integer that represents either `true` or `false` | Only the internal value used for operations is a 1-bit unsigned integer. An 8-bit unsigned integer is used in memory. |
| `string`* | A pointer to an array of 8-bit signed integers | Strings are null terminated. String literals are stored as global constants. |
| `i8` | An 8-bit signed integer | |
| `i16` | A 16-bit signed integer | |
| `i32` | A 32-bit signed integer | |
| `i64` | A 64-bit signed integer | |
| `f32` | A 32-bit floating point value | Uses IEEE 754 semantics |
| `f64` | A 64-bit floating point value | Uses IEEE 754 semantics |

*\*Support for these types is incomplete*

### 5.2 Casting matrix

The following matrix displays which types can be casted to which types (rows are the type of the value to cast, columns are the target type of the cast):

| | `void` | `bool` | `string` | `i8` | `i16` | `i32` | `i64` | `f32` | `f64` |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| `void` | | | | | | | | | |
| `bool` | | | | | | | | | |
| `string` | | | | | | | | | |
| `i8` | | x | | | x | x | x | x | x |
| `i16` | | x | | x | | x | x | x | x |
| `i32` | | x | | x | x | | x | x | x |
| `i64` | | x | | x | x | x | | x | x |
| `f32` | | | | x | x | x | x | | x |
| `f64` | | | | x | x | x | x | x | |

## 6. Operators

### 6.1 Binary Operators

| Operator | Precedence | Supported types |
| :- | :- | :- |
| < | 10 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| > | 10 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| == | 10 | `bool`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| != | 10 | `bool`, `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| <= | 10 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| >= | 10 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| + | 20 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| - | 20 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| * | 30 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
| / | 30 | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |

> [!note]
> Floating point comparisons use the unordered llvm comparisons, which means that the operands can be QNAN (quiet Not-a-number)
> -> QNAN means that operations with such a number generally don't raise exceptions

### 6.2 Unary Operators

| Operator | Supported types |
| :- | :- |
| - | `i8`, `i16`, `i32`, `i64`, `f32`, `f64` |
