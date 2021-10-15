#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef int64_t lith_Cell;
#define CELL lith_Cell

static const CELL LITH_NIL = 0;

enum {
  LITH_EXN_None = 0,
  LITH_EXN_Error,
  LITH_EXN_TypeMismatch,
  LITH_EXN_DivByZero,
  LITH_EXN_StackBounds,
  LITH_EXN_MemBounds,
  LITH_EXN_LookupFailure,
  LITH_EXN_BadNestLevel,
  LITH_EXN_COUNT_,
};

typedef struct lith_State_s lith_State;

typedef struct
{
  int memLimit;
  int dataStackLimit;
  int retStackLimit;
} lith_CreateOptions;

lith_State *lith_create(const lith_CreateOptions *opt);

void lith_destroy(lith_State *st);

void lith_dumpMem(lith_State *st, FILE * outFile);

/* Cell encoding --------------------------------------------------------------

A Cell can be NIL, Val, or Ptr, and a Ptr can be a Even or Atom.

NIL     00000000
Val     vvvvvvv1
Ptr     ppppppp0
  Even  pppppp00
  Atom  aaaaaa10


AtomStr nnnn0010
*/

// Assumes x fits in 63 bits
static inline CELL lith_makeVal(CELL x) { return (x << 1) | 1; }
static inline CELL lith_makePtr(CELL x) { return (x << 1) | 0; }
// Assumes x fits in 63 bits and even-aligned
static inline CELL lith_makeEven(CELL x) { return (x << 1) | 0; }
static inline CELL lith_makeAtom(CELL x) { return (x << 1) | 2; }

static inline CELL lith_toVal(CELL x) { return x | 1; }

// Cell decoding --------------------------------------------------------------

// Remove bottom bit to decode value
static inline CELL lith_getValOrPtr(CELL x) { return x >> 1; }
static inline CELL lith_getAtom(CELL x) { return x >> 2; }

// Cell type check ------------------------------------------------------------

// Cells can be a value or a pointer
static inline bool lith_isNull(CELL x) { return !x; }
static inline bool lith_isVal(CELL x) { return x && (x & 1); }
static inline bool lith_isPtr(CELL x) { return x && !(x & 1); }
// Further, pointers can be to a even or an atom
static inline bool lith_isEven(CELL x) { return x && ((x & 3) == 0); }
static inline bool lith_isAtom(CELL x) { return (x & 3) == 2; }

// Atoms ----------------------------------------------------------------------

#define lith_atomStr(a) ((const char *)&a + 1)

static inline int lith_atomLen(CELL a) { return (a >> 4) & 7; }

CELL lith_atomOfStr(const char *str, int strLen);

// Outer interpreter ----------------------------------------------------------

int lith_catch(lith_State * st, CELL xt);

// Inner interpreter ----------------------------------------------------------

void lith_interpLine(lith_State *st, const char *line, int lineLen);
