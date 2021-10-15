/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -S 1 src/prim.gperf  */
/* Computed positions: -k'1-3' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 3 "src/prim.gperf"

enum {
    LITH_PRIM_EXIT,
    LITH_PRIM_QUOT,
    LITH_PRIM_CALL,
    LITH_PRIM_GOTO,
    LITH_PRIM_ISNULL,
    LITH_PRIM_ISVAL,
    LITH_PRIM_ISPTR,
    LITH_PRIM_ISPAIR,
    LITH_PRIM_ISATOM,
    LITH_PRIM_ISEQUAL,
    LITH_PRIM_ISNEG,
    LITH_PRIM_ADD,
    LITH_PRIM_SUB,
    LITH_PRIM_MUL,
    LITH_PRIM_DIVMOD,
    LITH_PRIM_AND,
    LITH_PRIM_OR,
    LITH_PRIM_XOR,
    LITH_PRIM_DUP,
    LITH_PRIM_OVER,
    LITH_PRIM_DROP,
    LITH_PRIM_NIP,
    LITH_PRIM_TOR,
    LITH_PRIM_RFROM,
    LITH_PRIM_HERE,
    LITH_PRIM_ALLOT,
    LITH_PRIM_BIND,
    LITH_PRIM_FETCH,
    LITH_PRIM_STORE,
    LITH_PRIM_CIFETCH,
    LITH_PRIM_CISTORE,
    LITH_PRIM_PRINT,
    LITH_PRIM_CR,
};
#line 40 "src/prim.gperf"
struct resword_s { const char * name; int id; };
#include <string.h>

#define TOTAL_KEYWORDS 33
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 7
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 73
/* maximum key range = 72, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74,  5, 25, 10,
      10,  0, 15, 30, 50,  0, 74, 74,  0,  0,
      15,  0, 25, 10,  0, 30,  5, 15,  0, 74,
      20, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74, 74, 74, 74, 74,
      74, 74, 74, 74, 74, 74
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

struct resword_s *
in_word_set (register const char *str, register size_t len)
{
  static struct resword_s wordlist[] =
    {
#line 58 "src/prim.gperf"
      {"or", LITH_PRIM_OR,},
#line 61 "src/prim.gperf"
      {"over", LITH_PRIM_OVER},
#line 49 "src/prim.gperf"
      {"even?", LITH_PRIM_ISPAIR,},
#line 64 "src/prim.gperf"
      {"tor", LITH_PRIM_TOR,},
#line 47 "src/prim.gperf"
      {"val?", LITH_PRIM_ISVAL,},
#line 67 "src/prim.gperf"
      {"allot", LITH_PRIM_ALLOT,},
#line 74 "src/prim.gperf"
      {"cr", LITH_PRIM_CR,},
#line 62 "src/prim.gperf"
      {"drop", LITH_PRIM_DROP,},
#line 50 "src/prim.gperf"
      {"atom?", LITH_PRIM_ISATOM,},
#line 56 "src/prim.gperf"
      {"divmod", LITH_PRIM_DIVMOD,},
#line 55 "src/prim.gperf"
      {"mul", LITH_PRIM_MUL,},
#line 44 "src/prim.gperf"
      {"call", LITH_PRIM_CALL,},
#line 65 "src/prim.gperf"
      {"rfrom", LITH_PRIM_RFROM,},
#line 59 "src/prim.gperf"
      {"xor", LITH_PRIM_XOR,},
#line 42 "src/prim.gperf"
      {"exit", LITH_PRIM_EXIT,},
#line 69 "src/prim.gperf"
      {"fetch", LITH_PRIM_FETCH,},
#line 53 "src/prim.gperf"
      {"add", LITH_PRIM_ADD,},
#line 43 "src/prim.gperf"
      {"quot", LITH_PRIM_QUOT,},
#line 73 "src/prim.gperf"
      {"print", LITH_PRIM_PRINT,},
#line 51 "src/prim.gperf"
      {"equal?", LITH_PRIM_ISEQUAL,},
#line 71 "src/prim.gperf"
      {"cifetch", LITH_PRIM_CIFETCH,},
#line 57 "src/prim.gperf"
      {"and", LITH_PRIM_AND,},
#line 48 "src/prim.gperf"
      {"ptr?", LITH_PRIM_ISPTR,},
#line 46 "src/prim.gperf"
      {"null?", LITH_PRIM_ISNULL,},
#line 45 "src/prim.gperf"
      {"goto", LITH_PRIM_GOTO,},
#line 70 "src/prim.gperf"
      {"store", LITH_PRIM_STORE,},
#line 63 "src/prim.gperf"
      {"nip", LITH_PRIM_NIP,},
#line 68 "src/prim.gperf"
      {"bind", LITH_PRIM_BIND,},
#line 72 "src/prim.gperf"
      {"cistore", LITH_PRIM_CISTORE,},
#line 52 "src/prim.gperf"
      {"neg?", LITH_PRIM_ISNEG,},
#line 60 "src/prim.gperf"
      {"dup", LITH_PRIM_DUP,},
#line 66 "src/prim.gperf"
      {"here", LITH_PRIM_HERE,},
#line 54 "src/prim.gperf"
      {"sub", LITH_PRIM_SUB,}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register struct resword_s *resword;

          switch (key - 2)
            {
              case 0:
                resword = &wordlist[0];
                goto compare;
              case 2:
                resword = &wordlist[1];
                goto compare;
              case 3:
                resword = &wordlist[2];
                goto compare;
              case 6:
                resword = &wordlist[3];
                goto compare;
              case 7:
                resword = &wordlist[4];
                goto compare;
              case 8:
                resword = &wordlist[5];
                goto compare;
              case 10:
                resword = &wordlist[6];
                goto compare;
              case 12:
                resword = &wordlist[7];
                goto compare;
              case 13:
                resword = &wordlist[8];
                goto compare;
              case 14:
                resword = &wordlist[9];
                goto compare;
              case 16:
                resword = &wordlist[10];
                goto compare;
              case 17:
                resword = &wordlist[11];
                goto compare;
              case 18:
                resword = &wordlist[12];
                goto compare;
              case 21:
                resword = &wordlist[13];
                goto compare;
              case 22:
                resword = &wordlist[14];
                goto compare;
              case 23:
                resword = &wordlist[15];
                goto compare;
              case 26:
                resword = &wordlist[16];
                goto compare;
              case 27:
                resword = &wordlist[17];
                goto compare;
              case 28:
                resword = &wordlist[18];
                goto compare;
              case 29:
                resword = &wordlist[19];
                goto compare;
              case 30:
                resword = &wordlist[20];
                goto compare;
              case 31:
                resword = &wordlist[21];
                goto compare;
              case 32:
                resword = &wordlist[22];
                goto compare;
              case 33:
                resword = &wordlist[23];
                goto compare;
              case 37:
                resword = &wordlist[24];
                goto compare;
              case 38:
                resword = &wordlist[25];
                goto compare;
              case 41:
                resword = &wordlist[26];
                goto compare;
              case 42:
                resword = &wordlist[27];
                goto compare;
              case 45:
                resword = &wordlist[28];
                goto compare;
              case 47:
                resword = &wordlist[29];
                goto compare;
              case 51:
                resword = &wordlist[30];
                goto compare;
              case 52:
                resword = &wordlist[31];
                goto compare;
              case 71:
                resword = &wordlist[32];
                goto compare;
            }
          return 0;
        compare:
          {
            register const char *s = resword->name;

            if (*str == *s && !strcmp (str + 1, s + 1))
              return resword;
          }
        }
    }
  return 0;
}
