#include "lith.h"
#include "prim.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Generate struct members for a CELL stack
#define MakeStack(prefix)   \
    CELL *prefix##Stack;    \
    int prefix##StackLimit; \
    int prefix##StackPtr

struct lith_State_s
{
    CELL *mem;
    int memLimit;

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

void lith_dump(lith_State *st)
{
    assert(st);

    for (int i = 0; i < st->memLimit; ++i)
    {
        if (i % 4 == 0)
            printf("\n%08X |", i);

        printf("  %016lX", st->mem[i]);
    }
    puts("");
}

// Stack operations -----------------------------------------------------------

#define MakePush(prefix, st) ((st)->prefix##Stack[(st)->prefix##StackPtr++])
#define MakePop(prefix, st) ((st)->prefix##Stack[--(st)->prefix##StackPtr])
#define MakePeek(prefix, st, n) ((st)->prefix##Stack[(st)->prefix##StackPtr - (n)])

#define DPush(st) MakePush(data, st)
#define DPop(st) MakePop(data, st)
#define DPeek(st, n) MakePeek(data, st, n)
#define DTop(st) DPeek(st, 1)
#define DNxt(st) DPeek(st, 2)

#define RPush(st) MakePush(ret, st)
#define RPop(st) MakePop(ret, st)
#define RPeek(st, n) MakePeek(ret, st, n)

#define Mem(st, a) ((st)->mem[lith_getValOrPtr(a)])

// Inner interpreter ------------------------------------------------------------

static void doQuot(lith_State *st)
{
    CELL len = DTop(st);
    DTop(st) = st->rIP;
    st->rIP += len;
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
    DTop(st) = lith_makeVal(result.quot);
    DNxt(st) = lith_makeVal(result.rem);
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
    else
    {
        printf("$%016lx ", x);
    }
}

static void doCIFetch(lith_State *st)
{
    CELL offs = DTop(st);
    CELL addr = DNxt(st);
    DTop(st) = ((unsigned char *)&Mem(st, addr))[offs];
}

static void doCIStore(lith_State *st)
{
    CELL data = DTop(st);
    CELL offs = DNxt(st);
    CELL addr = DPeek(st, 3);
    DPop(st);
    DPop(st);
    ((unsigned char *)&Mem(st, addr))[offs] = data;
}

#define InHalfOpenRange(x, a, b) ((a) <= (x) && (x) < (b))
#define MakeStackCheck(prefix, st) (InHalfOpenRange((st)->prefix##StackPtr, 0, (st)->prefix##StackLimit))

static struct resword_s *hashAtom(CELL a)
{
    if (!lith_isAtom(a))
        return NULL;
    return in_word_set(lith_atomStr(a), lith_atomLen(a));
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
        while (lith_isPair(xt))
        {
            RPush(st) = st->rIP;
            xt = Mem(st, st->rIP);
            st->rIP += 2;
        }
        if (lith_isNull(xt))
        {
            st->rIP = RPop(st);
        }
        else if (lith_isVal(xt))
        {
            DPush(st) = xt;
        }
        else
        {
            assert(lith_isAtom(xt));
            struct resword_s *resword = hashAtom(xt);
            if (!resword)
            {
                fprintf(stderr, "cannot execute primitive %*s\n", lith_atomLen(xt), lith_atomStr(xt));
                assert(0 && "illegal primitive");
            }
            switch (resword->id)
            {
            // clang-format off
            // control flow
            case LITH_PRIM_EXIT: st->rIP = RPop(st); break;
            case LITH_PRIM_CALL: lith_call(st, DPop(st)); break;
            case LITH_PRIM_GOTO: st->rIP = DPop(st); break;
            case LITH_PRIM_QUOT: doQuot(st); break; // ( len -- addr )
            // type check
            case LITH_PRIM_ISNULL: DoUnaryFn(st, lith_isNull); break;
            case LITH_PRIM_ISVAL: DoUnaryFn(st, lith_isVal); break;
            case LITH_PRIM_ISPTR: DoUnaryFn(st, lith_isPtr); break;
            case LITH_PRIM_ISPAIR: DoUnaryFn(st, lith_isPair); break;
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
            // comma
            case LITH_PRIM_HERE: DPush(st) = lith_makePtr(st->rHere); break;
            case LITH_PRIM_ALLOT: st->rHere += lith_getValOrPtr(DPop(st)); break;
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
        DPush(st) = lith_makeVal(n);
        break;
    }
    case '&': // addr of word
    {
        assert(0);
        // CELL a = lith_atomOfStr(word + 1, wordLen - 1);
        // assert(!lith_isNull(a));
        // DPush(st) = a;
        break;
    }
    default: // word
    {
        CELL a = lith_atomOfStr(word, wordLen);
        assert(!lith_isNull(a));
        lith_call(st, a);
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
        lith_interpWord(st, word, strlen(word));
    }
}
