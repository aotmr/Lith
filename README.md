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

| Category | Name | Arity | Description |
|-|-|-|-|
| control flow |
| type check |
| comparison |
| arithmetic, logic |
| comma |
| memory access |
| output |