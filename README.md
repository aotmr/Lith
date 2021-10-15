# Lith: Test-Type Meta Environment

Lith is the successor to Elen,
recounting many of its ideas
(predominantly that of having typed memory cells),
but written in C instead of Awk.

In its current state, Lith is not only not Turing-complete,
it is missing major functionality and any amount of test code.
That will not be the case for much longer,
for a time scale of, hopefully, only a few hours.

# Building

## Dependencies

- C99 compiler
- histedit

# Design

## Cell Tags

Distinct from most Forths,
Lith has a weak dynamic typing system.
Cells are signed 64-bit integers with the bottom bit reserved as a tag,
with `Ptr`s being further distinguished
by the next least significant bit.

| Type | | Least Sig. Byte | Description |
|-|-|-|-|
| `NIL` | | `... 00000000` | (all zeroes) |
| `Val` | | `... _______1` | Integer |
| `Ptr` | | `... _______0` | Pointer |
| can be | `Pair` | `... ______00` | Pointer to secondary |
| can be | `Atom` | `... ______10` | Short string |

## Primitives

Lith currently has 26 primitives.

| Category | Name | Arity | Description |
|-|-|-|-|
| control flow | `exit` | ( -- ) | Return to the callee |
| | `quot` | ( len -- cell ) | Push a quotation of `len` cells to the stack |
| | `call` | ( xt -- ) | Call code at `xt`, which can be `Pair` or `Atom` |
| | `goto` | ( xt -- ) | Tail call to cell at address `xt` (must be a `Pair`) |
| type check | `null?` | ( cell -- flag ) |
| | `val?` | ( cell -- flag ) |
| | `ptr?` | ( cell -- flag ) |
| | `pair?` | ( cell -- flag ) |
| | `atom?` | ( cell -- flag ) |
| comparison | `equal?` | ( a b -- flag ) |
| | `neg?` | ( a -- flag ) |
| arithmetic | `add` | ( a b -- a+b ) |
| | `sub` | ( a b -- a-b ) |
| | `mul` | ( a b -- a*b) |
| | `divmod` | ( a b -- a/b a%b ) |
| bitwise | `and` | ( a b -- a&b ) |
| | `or` | ( a b -- a\|b ) |
| | `xor` | ( a b -- a^b ) |
| comma | `here` | ( -- addr ) | Push here-pointer |
| | `allot` | ( n -- ) | Adjust here-pointer by `n` cells |
| memory access | `fetch` | ( addr -- cell )
| | `store` | ( cell addr -- ) |
| | `cifetch` | ( addr offs -- addr byte ) | Fetch `byte` at `offs` bytes from the beginning of the cell at `addr`
| | `cistore` | ( addr offs byte -- addr ) | Store `byte` to `offs` bytes from the beginning of the cell at `addr`
| output | `print` | ( cell -- ) | Print a cell in a way that's more useful for debugging than anything
| | `cr` | ( -- ) | Print a newline