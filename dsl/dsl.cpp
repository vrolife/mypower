#include "dsl.hpp"

#include "expr.hpp"
#include "cmd.hpp"

namespace dsl {
    uint64_t parse_expr(const std::string& string) {
        return expr::parse(string);
    }

    std::vector<std::string> parse_command(const std::string& string) {
        return cmd::parse(string);
    }
}
