# Known issues

## 1. Compile errors in the condition of an `if` statement when nested binary expressions are used because of no type deduction

The language doesn't have type deduction.
This is completely fine in most cases, but the conditions of `if` statements can produce some unexpected errors in niche scenarios when binary expression with literals are used.
For instance, the following piece of code...

```
if (0 + 0 > 0.0)
```

...results in `[ Error ]: No implementation of binary operator '>' between types 'i32' and 'f32'` -- in theory, there should be no error, because all literals can evaluate type `f32`.

A bit of a technical explanation (doubt anyone is interested in this, but hey, here we go):\
Typechecking works in the following way:
An expression that knowns which types its child expressions should evaluate to requests that type from its children during typechecking (e. g. assignment to variable of type `i32` -> the child expression of the assignment has to evaluate to `i32`, so the assignment expression requests that type when typechecking)

```
i32 x               = 0
^                   ^ ^
target type is i32  │ │
                    │ │
                    │ target value is an integer literal
                    │
                    assignment requests the literal to evaluate to type i32
```

In case of a binary expression, this results in the following:

```
i32 x               = y + 10
^                     ^ ^ ^
target type is i32    │ │ │
                      │ │ │
                      │ binary expression requests lhs and rhs expressions to each evaluate to type i32
                      │   │
                      has to evaluate to i32
                          │
                          has to evaluate to i32
```

However, the condition of an `if` statement has to evaluate to type `bool`.
Directly using `bool` as the requested type, however, would cause a binary expression to try to evaluate its lhs and rhs expressions to `bool` respectively, which is not desired.

```
i32 x = 0
i32 y = 0
if                                                (x < y)
^                                                  ^ ^ ^
if statement requests type bool from the condition │ │ │
                                                   │ │ │
                                                   │ binary expression requests lhs and rhs expression to each evaluate to type bool
                                                   │   │
                                                   has to evaluate to bool, which is not correct
                                                       │
                                                       has to evaluate to bool, which is not correct
```

That's why the condition of an `if` statement leaves it up to the condition itself which type it wants to evaluate to and then just checks if the result is a `bool` -- so that binary expressions (and some other expressions) can correctly evaluate.

```
i32 x = 0
i32 y = 0
if                                           (x < y)
^                                             ^ ^ ^
if statement leaves the type to the condition │ │ │
│                                             │ │ │
│                                             │ binary expression also leaves evaluation to the expressions, because it doesn't know which type to produce
│                                             │ │ │
│                                             does whatever the fuck it wants, so it evaluates to i32
│                                               │ │
│                                               │ also does whatever the fuck it wants, it so evaluates to i32
│                                               │
│                                               binary expression checks if the types implement the operator <
│                                               
if statement checks if the resulting type is bool
```

However, in the first example...

```
if (0 + 0 > 0.0)
```

...all three literal expression evaluate to the type they want to evaluate to, which is `i32` for the integer literals and `f32` for the float literal -- which causes the mentioned error.

There are some rules in which order the expressions of a binary expression should be evaluated to avoid these kinds of type mismatches.
However, these rules are not recursive, so nested binary expressions pose a problem.
