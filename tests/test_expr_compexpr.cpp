#include "dsl.hpp"
#include "mathexpr.hpp"

using namespace mathexpr;

inline
uintptr_t compile_and_run(const std::string& code, uintptr_t old=0, uintptr_t _new=0, uintptr_t address=0) {
    auto comparator = dsl::parse_comparator_expression(code);
    auto exprcode = comparator.compile();
    return exprcode(old, _new, address);
}

int main(int argc, char *argv[])
{
    mathexpr::EXPR::_constant_optimal = false;

    assert(compile_and_run("=12", 0, 12, 0) == 1);
    assert(compile_and_run("!=12", 0, 12, 0) == 0);
    assert(compile_and_run("=12", 0, 11, 0) == 0);
    assert(compile_and_run("!=12", 0, 11, 0) == 1);

    // range
    assert(compile_and_run("=[(5+5),(2*10)]", 0, 12, 0) == 1);
    assert(compile_and_run("!=[(5+5),(2*10)]", 0, 12, 0) == 0);

    assert(compile_and_run("=[(5+5),(2*10)]", 0, 5, 0) == 0);
    assert(compile_and_run("=[(5+5),(2*10)]", 0, 21, 0) == 0);
    assert(compile_and_run("!=[(5+5),(2*10)]", 0, 5, 0) == 1);
    assert(compile_and_run("!=[(5+5),(2*10)]", 0, 21, 0) == 1);

    // mask
    assert(compile_and_run("={0xCC00,0xFF00}", 0, 0xCC99, 0) == 1);
    assert(compile_and_run("={0xAACC00,0xFF00}", 0, 0xBBCC99, 0) == 1);
    assert(compile_and_run("={0x00,0xFF}", 0, 0xCC, 0) == 0);
    assert(compile_and_run("!={0x00,0xFF}", 0, 0xCC, 0) == 1);
    return 0;
}