#include "mathexpr.hpp"

using namespace mathexpr;

inline
uintptr_t compile_and_run(const std::string& code) {
    auto ast = mathexpr::parse(code);
    auto* number = dynamic_cast<ASTNumber*>(ast.get());
    assert(number);
    return number->_value;
}

int main(int argc, char *argv[])
{
    assert(mathexpr::EXPR::_constant_optimal);
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
