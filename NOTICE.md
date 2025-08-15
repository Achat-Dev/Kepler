# Third party licenses

## Architecture

The architecture of the project is based on the [official LLVM tutorial](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/index.html)

> Copyright of the source code is held by the respective contributors\
> Licensed under the Apache-2.0 license with LLVM Exceptions\
> See the [license](https://github.com/llvm/llvm-project/blob/main/llvm/LICENSE.TXT) for more details
>
> Changes to the source code were made in form of:
>
> - Changed the return type of the `codegen` method
> - Changed `functions`, `if` and `for` expressions to have bodies
> - Changed parsing and lexing to fit the architecture of the project

## Included dependencies

(The links to the licenses only work if you correctly cloned the submodules)

**cxxopts** - 3.3.1

> Copyright (c) 2014 Jarryd Beck\
> Licensed under the MIT license\
> See the [license](./external/cxxopts/LICENSE) for more details

## Dependencies that are not included but need to be pre-installed

**LLVM** - 21.0.0git or newer

> Copyright of the source code is held by the respective contributors\
> Licensed under the Apache-2.0 license with LLVM Exceptions\
> See the [license](https://github.com/llvm/llvm-project/blob/main/LICENSE.TXT) for more details

**Clang++** - 20.1.8 or newer

> Copyright of the source code is held by the respective contributors\
> Licensed under the Apache-2.0 license with LLVM Exceptions\
> See the [license](https://github.com/llvm/llvm-project/blob/main/clang/LICENSE.TXT) for more details
