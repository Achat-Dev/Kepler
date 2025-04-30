# Kepler documentation

## Idea

The idea is to create a language that is a mix of Lua and Python.
The general syntax is taken from Lua with some concepts like OOP and operator overloading from Python mixed in.

However, the catch (and research reason) of this programming language is to be able to use both static and dynamic typing.

### Typing

The general syntax for typing is as follows:

`<type> <name> <expression>`

where:

| Component | Meaning | Further notes |
| :- | :- | :- |
| `<type>` | Either the data type (static typing) or `var` (dynamic typing) | If the whole statement is a variable definition, the variable can be made immutable by prefixing it with `const` (static typing) or by replacing `var` with `const` (dynamic typing) |
| `<name>` | The name of the thing in question (variable name, function name, ...) | |
| `<expression>` | In case of<br><ul><li>variable declaration: empty</li><li>variable definition: `= <value>`</li><li>function definition: the function arguments (in brackets) separated by commas. Function arguments are written like variable definitions</li></ul> | |

The possible static types are:

- `bool`
- `char`
- `string`
- `i8`
- `i16`
- `i32`
- `i64`
- `f32`
- `f64`

### Syntax

#### OOP

```
struct Name
  ...
end
```

#### if

```
if (<expression>)
  ...
elseif (<expression>)
  ...
else
  ...
end
```

#### while

```
while (<expression>)
  ...
end
```

#### for

```
for (type i : start, stop, step)
  ...
end
```

```
for (<type> item : list)
  ...
end
```

#### Methods

```
void foo()
  ...
end
```

```
int32 bar(int32 a, int32 b)
	return a + b
end
```

```
var baz()
  return ...
end
```

#### Casts

```
# <type>(<data>)
i32(5.4)
```

### Modules

Modules are handled similar to how Lua (and JavaScript) do it.
Some kind ob "object" that holds all the methods and variables is exported from the respective and imported from any other file.
Once a file is imported it will be compiled and executed on startup.

### Comments

```
# Single line comment
## Multi
line
comment ##
```
