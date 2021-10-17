#include "lith.h"
#include "prim.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ANSI escape sequences
#define EReset "\033[0m"
#define EBold "\033[1m"
#define ERev "\033[7m"

// Generate struct members for a CELL stack
#define MakeStack(prefix)   \
    CELL *prefix##Stack;    \
    int prefix##StackLimit; \
    int prefix##StackPtr

struct lith_State_s
{
    int iNest;

    CELL *mem;
    int memLimit;
    int dictLimit;

    CELL rLast;
    int rHere;
    int rIP;

    jmp_buf handleExn;
    bool catchExn;

    MakeStack(data);
    MakeStack(ret);
};

const lith_CreateOptions lith_defaultOptions = {
    .memLimit = 1 << 24,
    .dataStackLimit = 256,
    .retStackLimit = 256,
    .dictLimit = 1 << 16,
};

lith_State *lith_create(const lith_CreateOptions *opt)
{
    assert(opt);

    if (opt->memLimit < 0 ||
        opt->dataStackLimit < 0 ||
        opt->retStackLimit < 0 ||
        opt->dictLimit < 0 ||
        opt->memLimit < opt->dictLimit)
        return NULL;

    lith_State *st = calloc(1, sizeof(lith_State));
    assert(st);

    st->mem = calloc(opt->memLimit, sizeof(CELL));
    st->memLimit = opt->memLimit;
    st->dictLimit = opt->dictLimit;
    assert(st->mem);

    st->rLast = 0;
    st->rHere = 0x100;
    st->rIP = LITH_NIL;

    st->catchExn = false;

    st->dataStack = calloc(opt->dataStackLimit, sizeof(CELL));
    st->dataStackLimit = opt->dataStackLimit;
    assert(st->dataStack);

    st->retStack = calloc(opt->retStackLimit, sizeof(CELL));
    st->retStackLimit = opt->retStackLimit;
    assert(st->retStack);

    return st;
}

void lith_reset(lith_State *st)
{
    assert(st);

    st->rIP = 0;
    st->dataStackPtr = 0;
    st->retStackPtr = 0;
}

void lith_destroy(lith_State *st)
{
    assert(st);

    free(st->retStack);
    free(st->dataStack);
    free(st->mem);
    free(st);
}

// Exceptions -----------------------------------------------------------------

#define InHalfOpenRange(x, a, b) ((a) <= (x) && (x) < (b))

const char *lith_exnString(int error)
{
    static const char *exnMessages[LITH_EXN_COUNT_] = {
        "no error",
        "unspecified error",
        "type mismatch",
        "division by zero",
        "out of bounds stack access",
        "out of bounds memory access",
        "word lookup failure",
        "bad nest level",
    };
    return InHalfOpenRange(error, 0, LITH_EXN_COUNT_) ? exnMessages[error] : "unknown error";
}

void lith_throw(lith_State *st, int error)
{
    assert(st);

    if (st->catchExn)
    {
        longjmp(st->handleExn, error);
    }
    else
    {
        fprintf(stderr, "fatal: uncaught exception %d, %s\n", error, lith_exnString(error));
        exit(error);
    }
}

#define AssertThrow(st, expr, error) ((expr) ? (void)0 : lith_throw(st, error))

// Stack operations -----------------------------------------------------------

static inline CELL *peekImpl(lith_State *st, CELL *stack, int ptr, int limit, int n)
{
    assert(st);

    AssertThrow(st, InHalfOpenRange((ptr - n), 0, limit), LITH_EXN_StackBounds);
    return &stack[ptr - n];
}

#define MakePush(prefix, st) ((st)->prefix##Stack[(st)->prefix##StackPtr++])
#define MakePop(prefix, st) ((st)->prefix##Stack[--(st)->prefix##StackPtr])
#define MakePeek(prefix, st, n) (*peekImpl(st, (st)->prefix##Stack, (st)->prefix##StackPtr, (st)->prefix##StackLimit, (n)))
#define MakeStackCheck(prefix, st) (InHalfOpenRange((st)->prefix##StackPtr, 0, (st)->prefix##StackLimit))

#define DPush(st) MakePush(data, st)
#define DPop(st) MakePop(data, st)
#define DPeek(st, n) MakePeek(data, st, n)
#define DTop(st) DPeek(st, 1)
#define DNxt(st) DPeek(st, 2)

#define RPush(st) MakePush(ret, st)
#define RPop(st) MakePop(ret, st)
#define RPeek(st, n) MakePeek(ret, st, n)

#define Mem(st, a) ((st)->mem[lith_getValOrPtr(a)])

/* Dictionary -----------------------------------------------------------------

The dictionary is implemented as a contiguous associative array of cell pairs
that grows up to a limit below main memory. It is not inconceivable that we
could grow the dictionary in the negative direction as well.

We previously implemented this as an associative list of cons cells. This
proved to be wasteful (using four cells per entry) and the interlieving
of code and dictionary data makes any form of scoping impossible. Now it will
be possible to define nested colon definitions.

*/

#define AlignEven(st) ((st)->rHere += (st)->rHere & 1)
#define Comma(st) ((st)->mem[(st)->rHere++])

#define CAR(st, p) Mem(st, p + 0)
#define CDR(st, p) Mem(st, p + 2)

static void lith_bind(lith_State *st, CELL key, CELL val)
{
    assert(st);
    st->rLast += 2;
    CAR(st, lith_makePtr(st->rLast)) = key;
    CDR(st, lith_makePtr(st->rLast)) = val;
}

static CELL lith_find(lith_State *st, CELL key)
{
    assert(st);

    for (CELL p = lith_makePtr(st->rLast); p > 0; p -= 2)
    {
        if (CAR(st, p) == key)
            return CDR(st, p);
    }
    return LITH_NIL;
}

// Inner interpreter ------------------------------------------------------------

#define CIndex(st, addr, offs) (((unsigned char *)&Mem(st, addr))[lith_getValOrPtr(offs)])

#define DoUnaryFn(st, fn) (DTop(st) = fn(DTop(st)))
#define DoBinaryOp(st, op)                                  \
    do                                                      \
    {                                                       \
        int tag = DNxt(st) & 1;                             \
        DNxt(st) = (DNxt(st) & ~1) op(DTop(st) & ~1) | tag; \
        DPop(st);                                           \
    } while (0)

static void doQuot(lith_State *st)
{
    assert(st);

    CELL len = DTop(st);
    DTop(st) = st->rIP;
    st->rIP += lith_getValOrPtr(len);
}

static void doMul(lith_State *st)
{
    assert(st);

    CELL a = lith_getValOrPtr(DTop(st));
    CELL b = lith_getValOrPtr(DNxt(st));
    DNxt(st) = lith_makeVal(a * b);
    DPop(st);
}

static void doDivMod(lith_State *st)
{
    assert(st);

    CELL b = lith_getValOrPtr(DTop(st));
    CELL a = lith_getValOrPtr(DNxt(st));
    AssertThrow(st, b != 0, LITH_EXN_DivByZero); // throw on division by zeor

    ldiv_t result = ldiv(a, b);
    DTop(st) = lith_makeVal(result.rem);
    DNxt(st) = lith_makeVal(result.quot);
}

static void doPrintCell(CELL x)
{
    if (lith_isNull(x))
    {
        puts("NIL ");
    }
    else if (lith_isVal(x))
    {
        printf("#%ld ", lith_getValOrPtr(x));
    }
    else if (lith_isAtom(x))
    {
        printf("'%*s ", lith_atomLen(x), lith_atomStr(x));
    }
    else
    {
        printf("$%016lx ", x);
    }
}

static void doCIFetch(lith_State *st)
{
    assert(st);

    CELL offs = DTop(st);
    CELL addr = DNxt(st);
    DTop(st) = lith_makeVal(CIndex(st, addr, offs));
}

static void doCIStore(lith_State *st)
{
    assert(st);

    CELL data = DTop(st);
    CELL offs = DNxt(st);
    CELL addr = DPeek(st, 3);
    DPop(st);
    DPop(st);
    CIndex(st, addr, offs) = lith_getValOrPtr(data) & 0xFFu;
}

static bool cellIsProbablyAtom(CELL c)
{
    if (!lith_isAtom(c))
        return false;
    int atomLen = lith_atomLen(c);
    if (atomLen >= sizeof(CELL))
        return false;
    for (int i = 0; i < sizeof(CELL) - 1; ++i)
    {
        if (i < atomLen ? !isgraph(lith_atomStr(c)[i]) : lith_atomStr(c)[i] != '\0')
            return false;
    }
    return true;
}

static void prettyPrintCell(FILE *outFile, CELL c)
{
    static const int CELL_WIDTH = 16;
    if (lith_isNull(c))
    {
        fprintf(outFile, "%-*s  ", CELL_WIDTH, "NIL");
    }
    else if (lith_isVal(c))
    {
        fprintf(outFile, "#% *ld ", CELL_WIDTH, lith_getValOrPtr(c));
    }
    else if (cellIsProbablyAtom(c))
    {
        fprintf(outFile, "'%-*.*s ", CELL_WIDTH, lith_atomLen(c), lith_atomStr(c));
    }
    else
    {
        fprintf(outFile, "$%*lX ", CELL_WIDTH, c);
    }
}

void lith_dumpMem(lith_State *st, FILE *outFile)
{
    assert(st);

    static const int CELLS_PER_ROW = 4;

    fputc('\n', outFile);
    for (int i = 0; i < st->rHere && i < st->memLimit; i += CELLS_PER_ROW)
    {
        fprintf(outFile, "%08X |", (unsigned int)lith_makePtr(i));
        for (int j = 0; j < CELLS_PER_ROW; ++j)
        {
            prettyPrintCell(outFile, st->mem[i + j]);
        }
        fputc('\n', outFile);
    }
}

static struct resword_s *hashAtom(CELL a)
{
    if (!lith_isAtom(a))
        return NULL;
    return in_word_set(lith_atomStr(a), lith_atomLen(a));
}

static void dumpInnerState(lith_State *st, FILE *outFile, const char *info)
{
    assert(st);
    assert(info);

    fprintf(outFile, ERev "IP %08X\tDSP% 4d\tRSP% 4d\t%s\t" EReset, st->rIP, st->dataStackPtr, st->retStackPtr, info);
    for (int i = 0; i < st->dataStackPtr; ++i)
    {
        prettyPrintCell(outFile, st->dataStack[i]);
    }
    fputc('\n', outFile);
}

void lith_call(lith_State *st, CELL xt)
{
    assert(st);

    int retStackPtr0 = st->retStackPtr;
    // CALLing a NIL xt does nothing but stepping into it is the same as EXIT
    if (lith_isNull(xt))
        return;
    bool goOn = true;
    while (goOn)
    {
        while (lith_isEven(xt))
        {
            dumpInnerState(st, stderr, "Even");
            RPush(st) = st->rIP;
            st->rIP = xt;
            xt = Mem(st, st->rIP);
            st->rIP += 2;
        }
        if (lith_isNull(xt))
        {
            dumpInnerState(st, stderr, "NIL");
            st->rIP = RPop(st);
        }
        else if (lith_isVal(xt))
        {
            dumpInnerState(st, stderr, "Val");
            DPush(st) = xt;
        }
        else
        {
            assert(lith_isAtom(xt));
            struct resword_s *resword = hashAtom(xt);
            if (!resword)
            {
                fprintf(stderr, "%016lX\n", xt);
                fprintf(stderr, "cannot execute primitive %.*s\n", lith_atomLen(xt), lith_atomStr(xt));
                assert(0 && "illegal primitive");
            }
            switch (resword->id)
            {
            // clang-format off
            // control flow
            case LITH_PRIM_EXIT: st->rIP = RPop(st); break;
            case LITH_PRIM_IFEXIT: if (lith_getValOrPtr(RPop(st))) st->rIP = RPop(st); break;
            case LITH_PRIM_CALL: { CELL xt = DPop(st); lith_call(st, xt); } break;
            case LITH_PRIM_GOTO: st->rIP = DPop(st); break;
            case LITH_PRIM_QUOT: doQuot(st); break; // ( len -- addr )
            case LITH_PRIM_THROW: lith_throw(st, lith_getValOrPtr(DPop(st))); break;
            case LITH_PRIM_CATCH: DTop(st) = lith_catch(st, DTop(st)); break;
            // type check
            case LITH_PRIM_ISNULL: DoUnaryFn(st, lith_isNull); break;
            case LITH_PRIM_ISVAL: DoUnaryFn(st, lith_isVal); break;
            case LITH_PRIM_ISPTR: DoUnaryFn(st, lith_isPtr); break;
            case LITH_PRIM_ISEVEN: DoUnaryFn(st, lith_isEven); break;
            case LITH_PRIM_ISATOM: DoUnaryFn(st, lith_isAtom); break;
            // comparison
            case LITH_PRIM_ISEQUAL: DNxt(st) = lith_makeVal(-(DNxt(st) == DTop(st))); DPop(st); break;
            case LITH_PRIM_ISNEG: DTop(st) = lith_makeVal(-(DTop(st) < 0)); break;
            // arithmetic, logic
            case LITH_PRIM_ADD: DoBinaryOp(st, +); break;
            case LITH_PRIM_SUB: DoBinaryOp(st, -); break;
            case LITH_PRIM_MUL: doMul(st); break;
            case LITH_PRIM_DIVMOD: doDivMod(st); break;
            case LITH_PRIM_AND: DoBinaryOp(st, &); break;
            case LITH_PRIM_OR: DoBinaryOp(st, |); break;
            case LITH_PRIM_XOR: DoBinaryOp(st, ^); break;
            // stack
            case LITH_PRIM_DUP: DPush(st) = DTop(st); break;
            case LITH_PRIM_OVER: { CELL a = DNxt(st); DPush(st) = a; break; }
            case LITH_PRIM_DROP: DPop(st); break;
            case LITH_PRIM_NIP: DNxt(st) = DTop(st); DPop(st); break;
            case LITH_PRIM_TOR: RPush(st) = DPop(st); break;
            case LITH_PRIM_RFROM: DPush(st) = RPop(st); break;
            // comma
            case LITH_PRIM_HERE: DPush(st) = lith_makePtr(st->rHere); break;
            case LITH_PRIM_ALLOT: st->rHere += lith_getValOrPtr(DPop(st)); break;
            case LITH_PRIM_BIND: lith_bind(st, DTop(st), DNxt(st)); DPop(st); DPop(st); break;
            // memory access
            case LITH_PRIM_FETCH: DTop(st) = Mem(st, DTop(st)); break; // ( addr -- x )
            case LITH_PRIM_STORE: Mem(st, DTop(st)) = DNxt(st); DPop(st); DPop(st); break; // ( addr x -- )
            case LITH_PRIM_CIFETCH: doCIFetch(st); break; // ( addr offs -- addr c )
            case LITH_PRIM_CISTORE: doCIStore(st); break; // ( addr offs c -- addr )
            // output
            case LITH_PRIM_EMIT: putchar(lith_getValOrPtr(DPop(st))); break;
            case LITH_PRIM_PRINT: doPrintCell(DPop(st)); break;
            case LITH_PRIM_CR: puts(""); break;
            // clang-format on
            default:
                assert(0 && "illegal primitive number");
                __builtin_unreachable();
            }
            dumpInnerState(st, stderr, "Atom");
        }

        if ((goOn = st->retStackPtr > retStackPtr0))
        {
            xt = Mem(st, st->rIP);
            st->rIP += 2;
        }
    }
    AssertThrow(st, MakeStackCheck(data, st), LITH_EXN_StackBounds);
    AssertThrow(st, MakeStackCheck(ret, st), LITH_EXN_StackBounds);
}

int lith_catch(lith_State *st, CELL xt)
{
    assert(st);

    bool oldCatchExn = st->catchExn;
    jmp_buf oldHandleExn;
    memcpy(oldHandleExn, st->handleExn, sizeof(jmp_buf));

    st->catchExn = true;
    int result = setjmp(st->handleExn);
    if (result == 0)
    {
        lith_call(st, xt);
    }
    else
    {
        fprintf(stderr, "error: caught exception %d, %s\n", result, lith_exnString(result));
    }

    st->catchExn = oldCatchExn;
    memcpy(st->handleExn, oldHandleExn, sizeof(jmp_buf));
    return result;
}

// Atoms ----------------------------------------------------------------------

CELL lith_atomOfStr(const char *str, int strLen)
{
    assert(str);
    assert(strLen >= 0);

    if (strLen > sizeof(CELL) - 1)
        return LITH_NIL;
    CELL c = (strLen << 4) | 2;
    memcpy((char *)&c + 1, str, strLen);
    return c;
}

// Outer interpreter ----------------------------------------------------------

#define ConstAtom(str) lith_atomOfStr(str, strlen(str))

static CELL lookUpWord(lith_State *st, const char *word, int wordLen)
{
    assert(st);
    assert(word);
    assert(wordLen >= 0);

    CELL atom = lith_atomOfStr(word, wordLen);
    AssertThrow(st, !lith_isNull(atom), LITH_EXN_FailedAssertion);
    CELL addr = lith_find(st, atom);
    AssertThrow(st, !lith_isNull(addr), LITH_EXN_LookupFailure);
    return addr;
}

void compileOrPush(lith_State *st, CELL x)
{
    assert(st);

    if (st->iNest > 0)
        Comma(st) = x | 1;
    else
        DPush(st) = x;
}

void compileOrCall(lith_State *st, CELL x)
{
    assert(st);

    if (st->iNest > 0)
        Comma(st) = x;
    else
        lith_call(st, x);
}

// Interpret a single word
void lith_interpWord(lith_State *st, char *word, int wordLen)
{
    assert(st);
    assert(word);
    assert(wordLen >= 0);

    if (wordLen < 1)
        return;
    switch (word[0])
    {
    case '\'': // atom
    {
        // atom
        for (int i = 1; i < wordLen; ++i)
            if (word[i] == '_')
                word[i] = ' ';
        CELL a = lith_atomOfStr(word + 1, wordLen - 1);
        assert(a);
        DPush(st) = a;
        break;
    }
    case '#': // literal
    {
        long long int n = strtoll(word + 1, NULL, 0);
        CELL v = lith_makeVal(n);
        compileOrPush(st, v);
        break;
    }
    case '&': // addr of word
    {
        CELL addr = lookUpWord(st, word + 1, wordLen - 1);
        compileOrPush(st, addr);
        break;
    }
    case ':': // easy binding
    {
        AssertThrow(st, st->iNest == 0, LITH_EXN_BadNestLevel);
        ++st->iNest;

        CELL atom = lith_atomOfStr(word + 1, wordLen - 1);
        DPush(st) = atom;

        AlignEven(st);
        DPush(st) = lith_makePtr(st->rHere);
        break;
    }
    case ';': // exit or tail call
    {
        AssertThrow(st, st->iNest == 1, LITH_EXN_BadNestLevel);
        CELL addr = DPop(st);
        CELL atom = DPop(st);

        if (wordLen == 1)
        {
            Comma(st) = ConstAtom("exit");
        }
        else
        {
            CELL tail = lookUpWord(st, word + 1, wordLen - 1);
            AssertThrow(st, lith_isPtr(tail), LITH_EXN_TypeMismatch);
            Comma(st) = lith_toVal(tail);
            Comma(st) = ConstAtom("goto");
        }
        lith_bind(st, atom, addr);
        --st->iNest;
        break;
    }
    case '[': // open quotation
    {
        if (wordLen == 1)
        {
            ++st->iNest;
            // compile quotation
            DPush(st) = lith_makePtr(st->rHere); // fixup address
            Comma(st) = LITH_NIL;
            Comma(st) = ConstAtom("quot");

            // The body must be aligned as a `Even` for the interpreter to
            // recognize the address not as an `Atom`. This is why we push
            // a separate address for the body.
            AlignEven(st);
            DPush(st) = lith_makePtr(st->rHere); // body address
            break;
        }
        else
            goto default_;
    }
    case ']': // close quotation
    {
        if (wordLen == 1)
        {
            CELL body = DPop(st);
            CELL fix = DPop(st);
            Comma(st) = ConstAtom("exit");
            Mem(st, fix) = lith_makeVal(st->rHere - lith_getValOrPtr(fix));
            --st->iNest;
            assert(st->iNest >= 0);

            if (st->iNest == 0)
                DPush(st) = body;
            break;
        }
        else
            goto default_;
    }
    default_:
    default: // word
    {
        CELL a = lith_atomOfStr(word, wordLen);
        assert(!lith_isNull(a));
        CELL b = lith_find(st, a);
        compileOrCall(st, lith_isNull(b) ? a : b);
        break;
    }
    }
}

// Tokenize and execute a line of code
int lith_interpLine(lith_State *st, const char *line, int lineLen)
{
    assert(st);
    assert(line);
    assert(lineLen >= 0);

    if (lineLen == 0)
        return 0;

    char lineCopy[lineLen + 1];
    memcpy(lineCopy, line, lineLen);
    lineCopy[lineLen] = '\0';

    assert(!st->catchExn);
    st->catchExn = true;
    int error = setjmp(st->handleExn);
    if (error)
        return error;

    char *linePtr = lineCopy;
    char *word = NULL;
    while ((word = strsep(&linePtr, " \t\n\v\f\r")))
    {
        if (word[0] == '\\')
            break;
        lith_interpWord(st, word, strlen(word));
    }

    st->catchExn = false;
    return 0;
}
