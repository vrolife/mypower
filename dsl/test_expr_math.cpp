#include "expr.hpp"

using namespace expr;

int main(int argc, char *argv[])
{
    assert(parse("0x123+100") == (0x123 + 100));
    assert(parse("0x123-100") == (0x123 - 100));
    assert(parse("0x123*100") == (0x123 * 100));
    assert(parse("0x123/100") == (0x123 / 100));
    assert(parse("0x123%100") == (0x123 % 100));

    assert(parse("0x10+0x3*0o5") == 0x1F);
    assert(parse("(0x10+0x3)*0o5") == ((0x10+0x3)*05));

    assert(parse("1+3&1") == 0);
    assert(parse("2&4|8") == (2&4|8));
    assert(parse("~1+2") == 0);
    assert(parse("1+3<<1") == 8);
    assert(parse("2>1|2") == 3);
    return 0;
}