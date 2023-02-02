/*
Copyright (C) 2023 pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <sstream>

#include "mathexpr.hpp"
#include "mathexpr_parser.hpp"
#include "mathexpr_tokenizer.hpp"

namespace mathexpr {

bool EXPR::_constant_optimal = true;

struct ExprContext { };

std::unique_ptr<ASTNode> parse(const std::string& str)
{
    std::istringstream iss { str };
    Tokenizer tokenizer { "<commandline>", iss, std::cout };
    Parser<ExprContext> parser {};
    ExprContext context {};
    return parser.parse(context, tokenizer);
}

uintptr_t parse_address_or_throw(const std::string& str)
{
    auto addr_ast = mathexpr::parse(str);
    auto addr_number_ast = dynamic_cast<mathexpr::ASTNumber*>(addr_ast.get());
    if (addr_ast == nullptr) {
        throw std::out_of_range("Invalid address expression");
    }
    return addr_number_ast->_value;
}

} // namespace dsl
