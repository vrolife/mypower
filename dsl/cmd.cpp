#include <sstream>

#include "cmd.hpp"
#include "cmd_parser.hpp"
#include "cmd_tokenizer.hpp"

struct ParserContext { };

namespace cmd {

std::pair<std::string, std::vector<std::string>> parse(const std::string& str)
{
    std::istringstream iss{str};
    Tokenizer tokenizer{"<commandline>", iss, std::cout};
    Parser<ParserContext> parser{};
    ParserContext context{};
    return parser.parse(context, tokenizer);
}

} // namespace dsl
