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
    assert(compile_and_run("$old", 13, 12, 3) == 13);
    assert(compile_and_run("$new", 1, 2, 3) == 2);
    assert(compile_and_run("$addr", 1, 2, 3) == 3);
    assert(compile_and_run("$old+1", 99) == 100);
    return 0;
}