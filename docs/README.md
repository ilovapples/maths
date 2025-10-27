# Syntax Guide for [My Math Lang (MML)](https://github.com/ilovapples/maths)

### <span id="contents">Contents</span>:
1. [Concepts](#concepts)
2. [Basic Syntax](#basic-syntax)
3. [Advanced Syntax](#advanced-syntax)
4. [Built-ins](#built-ins)

## <span id="concepts">Concepts</span> [↩](#contents)
MML is a sort of mathematical scripting language with support for several mathematic data types, including real and complex numbers, vectors, and Booleans, with more planned (?) for the future.

There is a sort of 'concept' (hence the heading) that needs to be cleared up. In MML, when an expression is assigned to a variable, like `x = 9 + 3`, it is _literally_ assigned to the variable. This is distinct from the behavior of most _programming_
languages, which would in this situation evaluate the expression and assign the _value_ to the variable, not the expression itself. This decision was made 1. because I didn't have a great plan going into this project, and 2. because I think this 
behavior better reflects what would observed in an algebraic system. This interesting design allows for things like the following to be allowed (using python highlighting because it's probably easier to read):
```python
C = (F - 32) * 5/9
F = 80
C === 26 + 2/3
```
Notice that `F` wasn't defined before the second line `F = 80`, and yet `C` was well defined by its definition regardless of being dependent on the value of `F`. This also means that, when `F` is changed and `C` is reevaluated, it will reflect
the new value of `F`, almost as if `C` were a function `C{F}` (functions in this form are planned, but work has not begun just yet).

A variable/expression is 'evaluated' when it is used anywhere other than a variable definition or in a vector literal (see [Advanced Syntax](#advanced-syntax) for more on vectors). The interactive prompt (accessed via `-I` or `--interactive` from
the executable) automatically evaluates the last expression in each input line, but nothing is automatically evaluated if using the pure library (or the `--expr=` and `--no-eval` options combined, which specify an expression to parse but not
evaluate).

(Also, I'm not sure if this is the best place to put this, but you can always check the [TODO.md](../TODO.md) for anything that's planned, though the language might be more technical than would be easily understandable. If you're still not sure
about something, check the [Issues page](https://github.com/ilovapples/maths/issues).)

## <span id="basic-syntax">Basic Syntax</span> [↩](#contents)
MML uses the most of the usual syntax for mathematical expressions, as well as a few operators taken from popular programming languages.
In this guide, the word 'expression' will be used to mean any valid operation or constant, where an operation may have as one of its operands another operation. The word `expr` in a specification (the leftmost part of an entry) will be used to represent any valid expression.
All expressions (equality or otherwise) found here evaluate to the boolean value `true`, for density of the explanation.
#### Some of the relevant operators:
- `A == B`	= estimated (~13 digits accurate) equality operator		(ex. `3 == 3`)
- `A != B`	= estimated (~13 digits accurate) inequality operator	(ex. `9 != 5`)
- `A === B`	= exact equality operator								(ex. `3 === 3`)
- `A !== B`	= exact inequality operator								(ex. `9 !== 5`)
- `!A`		= unary Boolean NOT operator							(ex. `!false == true`)
- `A < B`	= less than operator									(ex. `9 < 5 == false`)
- `A <= B`	= less than or equal operator							(ex. `9 <= 9 == true`)
- `A > B`	= greater than operator									(ex. `9 > 5 == true`)
- `A >= B`	= greater than or equal operator						(ex. `9 >= 9 == true`)
- `A + B`	= add operation											(ex. `9 + 5 == 14`)
- `A - B`	= subtract operation									(ex. `9 - 5 == 4`)
- `-A`		= unary negate operation								(ex. `-9 + 5 == -4`)
- `+A`		= this does nothing but it's not invalid				(ex. `+9 + 5 == 14`)
- `A * B`	= multiply operation									(ex. `9 * 5 == 45`)
- `A / B`	= divide operation										(ex. `9 / 5 == 1.8`)
- `A ^ B`	= exponentiation operator								(ex. `9 ^ 5 ^ 2 == 9^25`)
- `A % B`	= modulo (remainder) operator							(ex. `9 % 5 == 4`)
- `|A|`		= absolute value operator								(ex. `|5-9| == 4`)
- `(expr)`	= parentheses are always evaluated first				(ex. `9*(2+3) == 45`)

Because it would be a massive pain otherwise, you may specify multiple statements/expressions by separating them with a semicolon (`;`).

## <span id="advanced-syntax">Advanced Syntax</span> [↩](#contents)
~~No, there's no scientific notation yet (it's planned though).~~
There is now support for scientific notation. This addition means that expressions like `2e` will be interpreted as `2` rather than as `2*e`, so take care to remember that.

MML also supports the use of vectors of any length (it gets weird if the length is 0, though).
A vector may be created via this syntax for a vector literal:
`[A, B, C, ...]` where there may be a trailing comma following the last element. `[5, 15, 9.2]` is an example.
The values of a vector may be accessed through the `.` operator, in a zero-indexed fashion, like so: `[5, 15, 9.2].2 == 9.2`
There is also nothing stopping you from creating nested vectors, but they do not behave as a matrix would in mathematics.

A variable can be defined via the `=` operator, like so: `x = 0` <br/>
For more specifics on the semantics of variable assignment, see [Concepts](#concepts).

MML allows you to implicitly multiply two distinct values in certain circumstances by placing them next to each other.
Example: `5pi == 5*pi` <br />

MML supports complex numbers as well. A complex number can be defined as: `z = 5 + 3i`, where `i` is a built-in constant (see [Built-ins](#built-ins) for other built-in constants) that is multiplied by the number `3` and added to `5`. This was not made possible via any special syntax---really, under the hood, MML just promotes a number to a complex number if it used in an expression with a complex number, so all the example really does is promote `3` to the complex number `3+0i`, multiply it by `i` to create `0+3i`, and add `5` to make `5+3i`, which is assigned to `z` (sort of).

Additionally, built-in functions are provided, though user-defined functions are not yet implemented. Functions are called via this syntax:
`function_name{argument1, argument2, ...}`. Most functions take just 1 argument, but some take 2 or more. For example: `cos{1.5pi} == 0` (due to rounding errors, this statement evaluates to `false` if the `===` exact equality operator is used: `cos{1.5pi} === 0`).
A full catalogue of all provided built-in functions and their properties is available in [Built-ins](#built-ins).

## <span id="built-ins">Built-ins</span> [↩](#contents)
#### Built-in constants:
- `true` = the Boolean value `true` (value of `1 == 1`)
- `false` = the Boolean value `false` (value of `0 == 1`)
- `pi` = the mathematical constant `π` (the ratio of a circle's circumference to its diameter).
- `e` = the mathematical consant `e` (Euler's number).
- `phi` = the mathematical constant `φ` (the golden ratio)
- `i` = the imaginary unit `i` (the square root of -1)
- `nan` = NaN = not a number (error value)
- `inf` = infinity (can be made negative with `-inf`)

In the leftmost section of a function's entry in this list, `...` represents the possibility for any number of arguments to be passed to a particular function. All functions here take as arguments a real or complex number, unless otherwise specified.
#### Built-in functions (see [Advanced Syntax](#advanced-syntax) for function call syntax):
- `print{...}` = evaluates and prints the values of its arguments (which may be of any type), separated by spaces, with no newline following the final printed value.
- `println{...}` = evaluates and prints the value of its arguments (which may be of any type), separated by newlines, with a newline following the final printed value.
- `dbg{expr}` = prints the Abstract Syntax Tree (AST) construction of an expression `expr`. The expression is not evaluated.
- `root{b, e} OR root{b}` = returns the `e`th root of `b`. if `e` is not present (one argument), the second root of `b` is returned.
- `logb{a, b} OR logb{a}` = returns the base `b` logarith of `a`. If `b` is not present (one argument), the natural logarithm of `a` is returned (same as `ln{a}`).
- `atan2{y, x}` = performs the `atan2` function on its 2 _real_ arguments `y` and `x` (see [the wiki page](https://en.wikipedia.org/wiki/Atan2)).
- `sin{a}` = returns the trigonometric `sin` aka sine function of `a`.
- `cos{a}` = returns the trigonometric `cos` aka cosine function of `a`.
- `tan{a}` = returns the trigonometric `tan` aka tangent function of `a`.
- `asin{a}` = returns the trigonometric `arcsin` aka inverse sine function of `a`.
- `acos{a}` = returns the trigonometric `arccos` aka inverse cosine function of `a`.
- `atan{a}` = returns the trigonometric `arctan` aka inverse tangent function of `a`.
- `sinh{a}` = returns the hyperbolic trigonometric `sin` aka sine function of `a`.
- `cosh{a}` = returns the hyperbolic trigonometric `cos` aka cosine function of `a`.
- `tanh{a}` = returns the hyperbolic trigonometric `tan` aka tangent function of `a`.
- `asinh{a}` = returns the hyperbolic trigonometric `arcsin` aka inverse sine function of `a`.
- `acosh{a}` = returns the hyperbolic trigonometric `arccos` aka inverse cosine function of `a`.
- `atanh{a}` = returns the hyperbolic trigonometric `arctan` aka inverse tangent function of `a`.
- `ln{x}` = returns the natural logarithm (base e logarithm) of `x`.
- `log2{x}` = returns the base 2 logarithm of `x`.
- `log10{x}` = returns the base 10 logarithm of `x`.
- `csqrt{x}` = returns the square root of `x` regardless of its sign. 
- `sqrt{x}` = returns the square root of `x`. If `x` is a negative real number, returns `nan`; otherwise, behaves like `csqrt{x}`.
- `floor{x}` = returns the floor of the real argument `x` (the greatest integer less than or equal to `x`).
- `ceil{x}` = returns the ceiling of the real argument `x` (the least integer greater than or equal to `x`).
- `round{x}` = returns the closest integer to the real argument `x` (rounds away from 0).
- `conj{z}` = returns the complex conjugate of the complex argument `z` (see [the wiki page](https://en.wikipedia.org/wiki/Complex_conjugate)).
- `phase{z}` = returns the phase (argument) (as a real) of the complex argument `z` (see [the wiki page](https://en.wikipedia.org/wiki/Argument_(complex_analysis))).
- `real{z}` = returns the real component (as a real) of the complex argument `z`.
- `imag{z}` = returns the imaginary component (as a real) of the complex argument `z`.

#### Additional functions that may or may not be provided
- `config_set{ident, val}` = sets the value of the configuration option specified by `ident` to `val`. Valid types for `val` depend on the config option specified by `ident`. 
- `max{...}` = returns the greatest of its arguments, where each of its arguments must be a real number or a Boolean value (the `max` function makes little sense on unordered values such as complex numbers).
- `min{...}` = returns the least of its arguments, where each of its arguments must be a real number or a Boolean value (the `min` function makes little sense on unordered values such as complex numbers).
- `sort{v}` = returns a sorted copy of its first argument `v`, a vector
