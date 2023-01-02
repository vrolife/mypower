#include "expr.hpp"

using namespace expr;

int main(int argc, char *argv[])
{
    assert(parse("0x123") == 0x123);
    assert(parse("0b11") == 0x3);
    assert(parse("0o755") == 493);
    assert(parse("123") == 123);
    return 0;
}