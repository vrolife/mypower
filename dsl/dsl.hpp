#ifndef __dsl_hpp__
#define __dsl_hpp__

#include <string>
#include <vector>

namespace dsl {
    uint64_t parse_expr(const std::string& string);
    std::vector<std::string> parse_command(const std::string& string);
}

#endif
