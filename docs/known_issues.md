# Known issues

## 1. Possible compile error when having a literal as the first value in an if condition

If the condition expression of an `if` expression starts with a literal, e.g.:

```
# 0.0 is the first value in the if condition
if (0.0 < x)
  ...
end
```

there might be a type mismatch compile error.
This is because leading literals in the condition expression of an `if` expression are configured to evaluate to the default type of their type category (`f32` for values of the type category `floating point` like in the example).
If the variable `x` in the example has a type other than `f32`, the compile error will occur.

## 2. Negative values as the first value of an if condition

The following will cause the compile error: `[ Compile error ]: type mismatch: trying to create binary operation with types 'i32' and 'f32'`.

```
if (-1.0 < x)
  ...
end
```

This is because of the nature of how negative values are created (subtracting the positive value from x) and how the first value in an if will be codegened.
The given example will be evaluated to:

```
if ((0 - 1.0) < x)
  ...
end
```

Due to the fact that the first value in an if is configured to select the default type of the value type category, the `0` will select `i32` for its type.
`1.0` is of type category `floating point`, which is incompatible with values of type category `integer` without casting, which is why the compile error occurs.
