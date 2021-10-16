# Lith: Test-Type Meta Environment

Lith is the successor to Elen,
recounting many of its ideas
(predominantly that of having typed memory cells),
but written in C instead of Awk.

Lith is technically Turing-complete,
but in its current state,
it is missing major functionality and any decent amount of test code.
Atoms are limited to seven characters.

These limitations will not be the case for much longer,
over a time scale of, hopefully, only a few days.

## Syntax

The outer interpreter, or parser, is very simple.
Lines are divided into words:
one or more non-space characters separated by one or more whitespace characters
(as defined by `isspace`).
Words can contain any non-whitespace character,
but words starting with punctuation
(as defined by `ispunct`)
are considered *sigilated*.
An initial sigil dictates how the parser interprets the rest of a word.

| Sigil | Description |
|-|-|
| *none* | Call word |
| `'` | Push atom, underscores replaced with spaces. |
| `#` | Push literal |
| `&` | Push address of word |
| `:` | Easy binding |
| `;` | Exit or tail call |

## Examples

### Desktop Calculator

With signed 63-bit integers,
we have more than enough range to do most calculations in fixed point.
Here's an example of calculating the decimal value
of an approximation of pi to six digits.
Notice that this approximation so far matches the digits of pi.
```
? #1000000  #355 mul  #113 divmod  drop print
#3141592 ok.
```
Let's increase the precision to nine digits.
Notice how the approximation begins to diverge
from the true value of pi after the sixth digit.
```
? #1000000000  #355 mul  #113 divmod  drop print
#3141592920 ok.
```

With quotations and bindings,
we can define our own words.
```
? [ dup dup mul ] 'cube bind
? #3 cube print
#27 ok.
```

## Building

### Dependencies

- C99 compiler
- histedit

## Design

### Cell Tags

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
| can be | `Even` | `... ______00` | Pointer to secondary |
| can be | `Atom` | `... ______10` | Short string |

This representation is sure to change in the future.
For example, given the berth a 64-bit word offers us,
we might use the *upper* bits of a cell to further annotate it.

### Primitives

Lith currently has 38 primitives.
Primitives are represented by atoms of their name.

| Category | Name | Arity | Description |
|-|-|-|-|
| control flow | `exit` | ( -- ) | return to the callee |
| | `?exit` | ( flag -- ) | conditionally return to callee |
| | `quot` | ( len -- cell ) | push a quotation of `len` cells to the stack |
| | `call` | ( xt -- ) | call code at `xt`, which can be `Even` or `Atom` |
| | `goto` | ( xt -- ) | tail call to cell at address `xt` (must be a `Even`) |
| | `throw` | ( exc -- ) | throw an exception with the number `exc` |
| | `catch` | ( xt -- exc ) | call code at `xt`, catching thrown exceptions |
| type check | `null?` | ( cell -- flag ) |
| | `val?` | ( cell -- flag ) |
| | `ptr?` | ( cell -- flag ) |
| | `even?` | ( cell -- flag ) |
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
| stack manipulation | `dup` | ( a -- a a ) | duplicate top stack item |
| | `over` | ( a b -- a b a ) | copy second stack item to top |
| | `drop` | ( a -- ) | discard top stack item |
| | `nip` | ( a b -- b ) | discard second stack item |
| | `>r` | ( x -- R:x ) | transfer value from return to data stack |
| | `r>` | ( R:x -- x ) | transfer values from data to return stack |
| comma | `here` | ( -- addr ) | push here-pointer |
| | `allot` | ( n -- ) | adjust here-pointer by `n` cells |
| | `bind` | ( v k -- ) | bind `v` to `k` in global dictionary |
| memory access | `fetch` | ( addr -- cell ) |
| | `store` | ( cell addr -- ) |
| | `cifetch` | ( addr offs -- addr byte ) | fetch `byte` at `offs` bytes from the beginning of the cell at `addr` |
| | `cistore` | ( addr offs byte -- addr ) | store `byte` to `offs` bytes from the beginning of the cell at `addr` |
| output | `emit` | ( char -- ) | write a cell converted to a character to standard input |
| | `print` | ( cell -- ) | print a cell in a way that's more useful for debugging than anything |
| | `cr` | ( -- ) | print a newline |

#### cifetch, cistore

Those wishing for finely-grained memory access
should use the **c**haracter **i**ndexed **fetch** and **store** primitives.
These primitives leave the address on the stack for your convenience.

## Future

### Sigils

Prospective sigils might include those in this table.

| Sigil | Description |
|-|-|
| `?` | Conditional execution |

### Regimes

We can generalize and unify the ideas of memory and stacks with *regimes*.
A *regime* is a growable dynamic array of cells,
somewhat like a memory segment.
The top 16 bits of a `Ptr` designates its regime.
Rather than the stacks residing in a separate memory space,
we would designate regimes for each stack
and a regime for the basic dictionary.
Additional regimes would be mapped on demand.

Rather than distinguishing `Even`s from `Atom`s by their low bits,
we could distinguish them by their regime--
similar to how many Lisp machines had memory regions for each kind of object.
Different regimes could hold different types of objects just as well.

### To-Do

- [ ] Truthy and falsey values are represented inconsistently.
It should be fine to use `NIL` to mean "false"
but accept either `NIL` *or* `#0` as false
- [ ] Incorporate a unit testing framework and add to build script