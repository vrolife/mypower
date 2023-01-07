#include "mathexpr.hpp"

#include "testcommon.hpp"

using namespace mathexpr;

int main(int argc, char *argv[])
{
    mathexpr::EXPR::_constant_optimal = false;
    assert(compile_and_run("$old", 13, 12, 3) == 13);
    assert(compile_and_run("$new", 1, 2, 3) == 2);
    assert(compile_and_run("$addr", 1, 2, 3) == 3);
    assert(compile_and_run("$old+1", 99) == 100);
    return 0;
}