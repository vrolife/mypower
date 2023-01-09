#include "mathexpr.hpp"

#include "dsl.hpp"

using namespace mathexpr;

inline
uintptr_t compile_and_run(const std::string& code, bool _unsigned=false, uintptr_t old=0, uintptr_t _new=0, uintptr_t address=0) {
    auto exprcode = dsl::compile_math_expression(code, _unsigned);
    return exprcode(old, _new, address);
}

int main(int argc, char *argv[])
{
    mathexpr::EXPR::_constant_optimal = false;
    assert(compile_and_run("-1>0", false) == 0);
    assert(compile_and_run("-1>0", true) == 1);
    return 0;
}
