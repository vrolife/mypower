#include <sstream>

#include "expr.hpp"
#include "expr_parser.hpp"
#include "expr_tokenizer.hpp"

struct ParserContext { };

namespace expr {

uint64_t parse(const std::string& str)
{
    std::istringstream iss{str};
    Tokenizer tokenizer{"<commandline>", iss, std::cout};
    Parser<ParserContext> parser{};
    ParserContext context{};
    return parser.parse(context, tokenizer);
}

} // namespace dsl
