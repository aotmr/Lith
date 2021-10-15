#include "lith.h"
#include "prim.h"

#include <assert.h>
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

    CELL rLast;
    int rHere;
    int rIP;

    MakeStack(data);
    MakeStack(ret);
};

lith_State *lith_create(const lith_CreateOptions *opt)
{
    assert(opt);

    lith_State *st = calloc(1, sizeof(lith_State));
    assert(st);

    st->mem = calloc(opt->memLimit, sizeof(CELL));
    st->memLimit = opt->memLimit;
    assert(st->mem);

    st->rLast = LITH_NIL;
    st->rHere = 1;
    st->rIP = LITH_NIL;

    st->dataStack = calloc(opt->dataStackLimit, sizeof(CELL));
    st->dataStackLimit = opt->dataStackLimit;
    assert(st->dataStack);

    st->retStack = calloc(opt->retStackLimit, sizeof(CELL));
    st->retStackLimit = opt->retStackLimit;
    assert(st->retStack);

    return st;
}

void lith_destroy(lith_State *st)
{
    assert(st);

    free(st->retStack);
    free(st->dataStack);
    free(st->mem);
    free(st);
}

void lith_dumpMem(lith_State *st)
{
    assert(st);

    for (int i = 0; i < st->rHere && i < st->memLimit; ++i)
    {
        if (i % 4 == 0)
            printf("\n%08X |", i);

        printf("  %016lX", st->mem[i]);
    }
    puts("");
}

// Stack operations -----------------------------------------------------------

#define InHalfOpenRange(x, a, b) ((a) <= (x) && (x) < (b))

static inline CELL *peekImpl(lith_State *st, CELL *stack, int ptr, int limit, int n)
{
    assert(st);
    assert(InHalfOpenRange((ptr - n), 0, limit));
    return &stack[ptr - n];
}

#define MakePush(prefix, st) ((st)->prefix##Stack[(st)->prefix##StackPtr++])
#define MakePop(prefix, st) ((st)->prefix##Stack[--(st)->prefix##StackPtr])
#define MakePeek(prefix, st, n) (*peekImpl(st, (st)->prefix##Stack, (st)->prefix##StackPtr, (st)->prefix##StackLimit, (n)))

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

The dictionary is currently implemented as an associative list using cons cells
as the basic data structure. The dictionary is present in the image but only
the interpreter needs to touch it.

*/

#define AlignEven(st) ((st)->rHere += (st)->rHere & 1)
#define Comma(st) ((st)->mem[(st)->rHere++])

static CELL lith_cons(lith_State *st, CELL car, CELL cdr)
{
    AlignEven(st);
    int addr = st->rHere;
    Comma(st) = car;
    Comma(st) = cdr;
    return lith_makeEven(addr);
}

static void lith_bind(lith_State *st, CELL key, CELL val)
{
    st->rLast = lith_cons(st, lith_cons(st, key, val), st->rLast);
}

#define CAR(st, p) Mem(st, p + 0)
#define CDR(st, p) Mem(st, p + 2)

#define CAAR(st, p) CAR(st, CAR(st, p))
#define CADR(st, p) CDR(st, CAR(st, p))

static CELL lith_find(lith_State *st, CELL key)
{
    for (CELL p = st->rLast; !lith_isNull(p); p = CDR(st, p))
    {
        if (CAAR(st, p) == key)
            return CADR(st, p);
    }
    return LITH_NIL;
}

// Inner interpreter ------------------------------------------------------------

static void doQuot(lith_State *st)
{
    CELL len = DTop(st);
    DTop(st) = st->rIP;
    st->rIP += lith_getValOrPtr(len);
}

#define DoUnaryFn(st, fn) (DTop(st) = fn(DTop(st)))
#define DoBinaryOp(st, op)                                  \
    do                                                      \
    {                                                       \
        int tag = DNxt(st) & 1;                             \
        DNxt(st) = (DNxt(st) & ~1) op(DTop(st) & ~1) | tag; \
        DPop(st);                                           \
    } while (0)

static void doMul(lith_State *st)
{
    CELL a = lith_getValOrPtr(DTop(st));
    CELL b = lith_getValOrPtr(DNxt(st));
    DNxt(st) = lith_makeVal(a * b);
    DPop(st);
}

static void doDivMod(lith_State *st)
{
    CELL b = lith_getValOrPtr(DTop(st));
    CELL a = lith_getValOrPtr(DNxt(st));
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

#define CIndex(st, addr, offs) (((unsigned char *)&Mem(st, addr))[lith_getValOrPtr(offs)])

static void doCIFetch(lith_State *st)
{
    CELL offs = DTop(st);
    CELL addr = DNxt(st);
    DTop(st) = lith_makeVal(CIndex(st, addr, offs));
}

static void doCIStore(lith_State *st)
{
    CELL data = DTop(st);
    CELL offs = DNxt(st);
    CELL addr = DPeek(st, 3);
    DPop(st);
    DPop(st);
    CIndex(st, addr, offs) = lith_getValOrPtr(data) & 0xFFu;
}

#define MakeStackCheck(prefix, st) (InHalfOpenRange((st)->prefix##StackPtr, 0, (st)->prefix##StackLimit))

static struct resword_s *hashAtom(CELL a)
{
    if (!lith_isAtom(a))
        return NULL;
    return in_word_set(lith_atomStr(a), lith_atomLen(a));
}

static void dumpInnerState(lith_State *st, const char *info)
{
    fprintf(stderr, ERev "IP %08X\tDSP% 4d\tRSP% 4d\t%s\n" EReset, st->rIP, st->dataStackPtr, st->retStackPtr, info);
}

void lith_call(lith_State *st, CELL xt)
{
    int retStackPtr0 = st->retStackPtr;
    // CALLing a NIL xt does nothing but stepping into it is the same as EXIT
    if (lith_isNull(xt))
        return;
    bool goOn = true;
    while (goOn)
    {
        while (lith_isEven(xt))
        {
            dumpInnerState(st, "Even");
            RPush(st) = st->rIP;
            st->rIP = xt;
            xt = Mem(st, st->rIP);
            st->rIP += 2;
        }
        if (lith_isNull(xt))
        {
            dumpInnerState(st, "NIL");
            st->rIP = RPop(st);
        }
        else if (lith_isVal(xt))
        {
            dumpInnerState(st, "Val");
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
            case LITH_PRIM_CALL: RPush(st) = st->rIP; st->rIP = DPop(st); break;
            case LITH_PRIM_GOTO: st->rIP = DPop(st); break;
            case LITH_PRIM_QUOT: doQuot(st); break; // ( len -- addr )
            // type check
            case LITH_PRIM_ISNULL: DoUnaryFn(st, lith_isNull); break;
            case LITH_PRIM_ISVAL: DoUnaryFn(st, lith_isVal); break;
            case LITH_PRIM_ISPTR: DoUnaryFn(st, lith_isPtr); break;
            case LITH_PRIM_ISPAIR: DoUnaryFn(st, lith_isEven); break;
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
            case LITH_PRIM_PRINT: doPrintCell(DPop(st)); break;
            case LITH_PRIM_CR: puts(""); break;
            // clang-format on
            default:
                assert(0 && "illegal primitive number");
                __builtin_unreachable();
            }
            dumpInnerState(st, "Atom");
        }
        if ((goOn = st->retStackPtr > retStackPtr0))
        {
            xt = Mem(st, st->rIP);
            st->rIP += 2;
        }
    }

    assert(MakeStackCheck(data, st));
}

// Atoms ----------------------------------------------------------------------

CELL lith_atomOfStr(const char *str, int strLen)
{
    if (strLen > sizeof(CELL) - 1)
        return LITH_NIL;
    CELL c = (strLen << 4) | 2;
    memcpy((char *)&c + 1, str, strLen);
    return c;
}

// Outer interpreter ----------------------------------------------------------

void compileOrPush(lith_State *st, CELL x)
{
    if (st->iNest > 0)
        Comma(st) = x | 1;
    else
        DPush(st) = x;
}

void compileOrCall(lith_State *st, CELL x)
{
    if (st->iNest > 0)
        Comma(st) = x;
    else
        lith_call(st, x);
}

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
        CELL a = lith_atomOfStr(word + 1, wordLen - 1);
        assert(!lith_isNull(a));
        CELL b = lith_find(st, a);
        assert(!lith_isNull(b) && "cannot find word in dictionary");
        compileOrPush(st, b);
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
            Comma(st) = lith_atomOfStr("quot", 4);

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
            Comma(st) = lith_atomOfStr("exit", 4);
            Mem(st, fix) = lith_makeVal(st->rHere - lith_getValOrPtr(fix));

            --st->iNest;
            assert(st->iNest >= 0);

            compileOrPush(st, body);
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

void lith_interpLine(lith_State *st, const char *line, int lineLen)
{
    assert(st);
    assert(line);
    assert(lineLen >= 0);

    if (lineLen == 0)
        return;

    char lineCopy[lineLen + 1];
    memcpy(lineCopy, line, lineLen);
    lineCopy[lineLen] = '\0';

    char *linePtr = lineCopy;
    char *word = NULL;
    while ((word = strsep(&linePtr, " \t\n\v\f\r")))
    {
        if (word[0] == '\\')
            break;
        lith_interpWord(st, word, strlen(word));
    }
}
