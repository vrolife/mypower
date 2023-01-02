#include "cmd.hpp"

using namespace cmd;

int main(int argc, char *argv[])
{
    assert(parse("hello") == std::vector<std::string>{"hello"});
    assert(parse("hello world") == (std::vector<std::string>{"hello", "world"}));
    assert(parse("\"hello world\"") == std::vector<std::string>{"hello world"});
    assert(parse("hello\\ world") == std::vector<std::string>{"hello world"});
    return 0;
}
