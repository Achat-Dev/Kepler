# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-09-09

### Added

- Unsigned integer types (`u8`, `u16`, `u32` and `u64`)

### Changed

- Files are now cached after the first load to avoid multiple loads during diagnostic printing
- Diagnostics can span across multiple lines now
- Diagnostics for mathematical negations now include the minus sign

### Fixed

- Redundant casts reporting the wrong diagnostic
- Float casts being the wrong way round
- Diagnostics including literals printing the id of the literal instead of the actual literal
- Diagnostic of invalid mathematical negation printing the minus sign instead of the expression to negate

## [0.1.0] - 2026-09-03

### Added

- Block scopes for `if`, `elseif`, `else` and `for` statements
- Variadic functions (this feature is incomplete and only intended for compatibility with `C` code (there currently is no way to access the variadic arguments inside of a function))
- Options for different optimization levels:
  - `-O 0`: (Almost) no optimization
  - `-O 1`: Optimize quickly without destroying debuggability
  - `-O 2`: Optimize for fast execution as much as possible without triggering significant incremental compile time or code size growth
  - `-O 3`: Optimize for fast execution as much as possible no matter the compilation cost
  - `-O s`: Similar to `-O 2` but tries to optimize for small code size instead of fast execution
  - `-O z`: A very specialized mode that will optimize for code size at any and all costs

### Changed

- The `-v` option now prints the current version of the compiler instead of enabling verbose logging
- The `-a` option now accepts `.c` files instead of `.cpp` files
- Diagnostics are now collected and printed in their entirety at a specific point instead of terminating after the first diagnostic
- Improved diagnostic messages
- Functions can now be called before they are defined
- Reworked the architecture to a multi-pass compiler instead of a single-pass compiler
  - Moved ... from lexer, parser and ASTNodes into the following, separate passes
    - Missing return & unreachable code detection
    - Name resolution
    - Type checking
    - Code generation
  - Type checking now uses a proper type table
  - Name resolution now uses a proper symbol table
- Strings are now interned during the compilation to lower memory usage and increase performance
- Split assertions into multiple macros (and added a shit load of assertions)

### Removed

- `tmap` type
- All `string` operations (concatenation, comparison and casting to `string`)
- Garbage collection for dynamically allocated strings (because they don't exist anymore)
  - This also removed the dependency on `libgc`
- The custom runtime
  - This also removed the dependency on `xxd`
- Verbose logging (previous usage of the `-v` option)
- The technical documentation
- The easter eggs :')

### Fixed

- Linking the project with LLVM if LLVM is installed as multiple libraries instead of a single one (this mainly applies to Windows... I think)
- The compiler now checks if dependencies are installed before starting the compilation to avoid crashes at the end of the compilation (specifically when calling `clang` to link the final executable)

### Security

- Linking the executable is no longer vulnerable to shell injections

## [university-submission] - 2025-08-25

### Added

- Compilation of a single `.kpl` file as well as additional `.cpp` or `.o` files into a single executable
- Printing of a diagnostic if the compilation fails
- Basic types:
  - `void` type
  - `bool` type
  - `string` type
  - Integer types (`i8`, `i16`, `i32` and `i64`)
  - Floating point types (`f32` and `f64`)
  - `tmap` type (a struct with a field for each of the other types)
- Casting between types
- Basic operators:
  - `+`, `-`, `*`, `/`, `<`, `>`, `==`, `!=`, `<=`, `>=`
- Garbage collection for dynamically allocated strings created through concatenation (via `libgc`)
- Functions, `extern` functions and function calls
- Local variables inside of functions
- `if`, `elseif`, `else` and `for` statements for control flow
- Custom runtime with the following functions:
  - `error(string s)`
  - `print(string s)`
  - `pause()`
- Docs containing:
  - The installation process
  - A language overview
  - A technical documentation
  - Known issues
- A license for the source code
