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
    assert(compile_and_run("0x123") == 0x123);
    assert(compile_and_run("0b11") == 0x3);
    assert(compile_and_run("0o755") == 493);
    assert(compile_and_run("123") == 123);
    return 0;
}
