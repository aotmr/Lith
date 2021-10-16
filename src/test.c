#include "utest.h"
#include "lith.h"

#include <string.h>

#define TestLine(st, code) lith_interpLine((st), ("[ " code " ?exit throw ] call"));

struct BasicFixture
{
    lith_State *st;
};

#define ST (utest_fixture->st)
#define InterpLiteral(st, str) lith_interpLine((st), (str), strlen(str))

UTEST_F_SETUP(BasicFixture)
{
    utest_fixture->st = lith_create(&(lith_CreateOptions){1 << 16, 32, 32});
    EXPECT_TRUE(utest_fixture->st);
}

UTEST_F(BasicFixture, empty_line_0) { InterpLiteral(ST, ""); }
UTEST_F(BasicFixture, blank_line_1) { InterpLiteral(ST, " "); }
UTEST_F(BasicFixture, blank_line_9) { InterpLiteral(ST, "         "); }

UTEST_F_TEARDOWN(BasicFixture)
{
    lith_destroy(utest_fixture->st);
}

UTEST_MAIN();