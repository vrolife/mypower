#include "mathexpr.hpp"

#include "testcommon.hpp"

using namespace mathexpr;

int main(int argc, char *argv[])
{
    assert(compile_and_run("0x123") == 0x123);
    assert(compile_and_run("0b11") == 0x3);
    assert(compile_and_run("0o755") == 493);
    assert(compile_and_run("123") == 123);
    return 0;
}
