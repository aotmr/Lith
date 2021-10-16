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
    utest_fixture->st = lith_create(&lith_defaultOptions);
    EXPECT_TRUE(utest_fixture->st);
}

UTEST_F(BasicFixture, empty_line_0) { InterpLiteral(ST, ""); }
UTEST_F(BasicFixture, blank_line_1) { InterpLiteral(ST, " "); }
UTEST_F(BasicFixture, blank_line_9) { InterpLiteral(ST, "         "); }

UTEST_F(BasicFixture, can_throw_manually) { EXPECT_EQ(InterpLiteral(ST, "#0 throw"), LITH_EXN_FailedAssertion); }
UTEST_F(BasicFixture, drop_throws_on_underflow) { EXPECT_EQ(InterpLiteral(ST, "drop"), LITH_EXN_StackBounds); }
UTEST_F(BasicFixture, over_throws_on_underflow) { EXPECT_EQ(InterpLiteral(ST, "over"), LITH_EXN_StackBounds); }
UTEST_F(BasicFixture, tor_throws_on_underflow) { EXPECT_EQ(InterpLiteral(ST, "tor"), LITH_EXN_StackBounds); }
UTEST_F(BasicFixture, rfrom_throws_on_underflow) { EXPECT_EQ(InterpLiteral(ST, "rfrom"), LITH_EXN_StackBounds); }

UTEST_F(BasicFixture, interpret_quotation) { EXPECT_EQ(-3, InterpLiteral(ST, "[ #-3 ] call throw")); }
UTEST_F(BasicFixture, nested_quotations_1) { EXPECT_EQ(-8, InterpLiteral(ST, "[ [ #-4 dup ] ] call call add throw")); }
UTEST_F(BasicFixture, nested_quotations_2) { EXPECT_EQ(125, InterpLiteral(ST, "#105 [ [ #20 add throw ] ] call call")); }

UTEST_F_TEARDOWN(BasicFixture)
{
    lith_destroy(utest_fixture->st);
}

UTEST_MAIN();