#include "mathexpr.hpp"

#include "dsl.hpp"

using namespace mathexpr;

inline
uintptr_t compile_and_run(const std::string& code, uintptr_t old=0, uintptr_t _new=0, uintptr_t address=0) {
    auto exprcode = dsl::compile_math_expression(code);
    return exprcode(old, _new, address);
}

int main(int argc, char *argv[])
{
    mathexpr::EXPR::_constant_optimal = false;
    assert(compile_and_run("0x123+100") == (0x123 + 100));
    assert(compile_and_run("0x123-100") == (0x123 - 100));
    assert(compile_and_run("0x123*100") == (0x123 * 100));
    assert(compile_and_run("0x123/100") == (0x123 / 100));
    assert(compile_and_run("0x123%100") == (0x123 % 100));

    assert(compile_and_run("0x10+0x3*0o5") == 0x1F);
    assert(compile_and_run("(0x10+0x3)*0o5") == ((0x10+0x3)*05));

    assert(compile_and_run("1+3&1") == 0);
    assert(compile_and_run("2&4|8") == (2&4|8));
    assert(compile_and_run("~1+2") == 0);
    assert(compile_and_run("1+3<<1") == 8);
    assert(compile_and_run("2>1|2") == 3);
    assert(compile_and_run("1?2:3") == 2);
    assert(compile_and_run("0?2:3") == 3);

    assert(compile_and_run("((10-2)+0x3)*((4+5)+(5-2))") == 132);
    return 0;
}
