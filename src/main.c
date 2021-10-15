#include "lith.h"

#include <argp.h>
#include <assert.h>
#include <histedit.h>
#include <stdio.h>

int main(int argc, char * * argv)
{
    error_t error = argp_parse(NULL, argc, argv, 0, NULL, NULL);
    (void)error;
    
    lith_State *st = lith_create(&(lith_CreateOptions){ 0x10000, 32, 32 });
    assert(st);

    EditLine * el = el_init(argv[0], stdin, stdout, stderr);
    assert(el);

    const char * line = NULL;
    int lineLen = 0;
    while ((line = el_gets(el, &lineLen))) {
        lith_interpLine(st, line, lineLen);
    }
    el_end(el);
    lith_destroy(st);
}