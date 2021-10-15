#include "lith.h"

#include <argp.h>
#include <assert.h>
#include <histedit.h>
#include <stdio.h>

// wrap with literal escape for editline
#define ELit(s) "\1" s "\1 "

// ANSI escape sequences
#define EReset "\033[0m"
#define EBold "\033[1m"
#define ERev "\033[7m"

static char *prompt(EditLine *el)
{
    static char promptStr[] = ELit(EBold "?" EReset);
    return promptStr;
}

int main(int argc, char **argv)
{
    error_t error = argp_parse(NULL, argc, argv, 0, NULL, NULL);
    (void)error;

    lith_State *st = lith_create(&(lith_CreateOptions){1 << 10, 32, 32});
    assert(st);

    EditLine *el = el_init(argv[0], stdin, stdout, stderr);
    assert(el);
    el_set(el, EL_PROMPT_ESC, prompt, '\1');

    const char *line = NULL;
    int lineLen = 0;
    while ((line = el_gets(el, &lineLen)))
    {
        lith_interpLine(st, line, lineLen);
        puts(ELit(EBold "ok." EReset));
    }
    el_end(el);

    lith_dumpMem(st);
    lith_destroy(st);
}