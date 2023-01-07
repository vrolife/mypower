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
#include <iostream>

#include "dsl.hpp"

#include "mathexpr.hpp"
#include "cmd.hpp"

#include "compexpr.hpp"
#include "compexpr_parser.hpp"
#include "compexpr_tokenizer.hpp"

namespace dsl {

static JITCode compile_ast(std::unique_ptr<mathexpr::ASTNode>&& ast) {
    void* code = nullptr;
    size_t length = 0;

    mathexpr::Compiler compiler{};

    compiler._local_size = 3;
    compiler._local_vars.emplace("$old", 0);
    compiler._local_vars.emplace("$new", 1 * sizeof(uintptr_t));
    compiler._local_vars.emplace("$addr", 2 * sizeof(uintptr_t));
    compiler._local_vars.emplace("$address", 2 * sizeof(uintptr_t));

    auto depth = ast->depth(0);

    sljit_emit_enter(compiler, 0, SLJIT_ARGS3(W, W, W, W), 3, 3, 0, 0, (compiler._local_size + depth) * sizeof(uintptr_t));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 0, SLJIT_S0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 1 * sizeof(uintptr_t), SLJIT_S1, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 2 * sizeof(uintptr_t), SLJIT_S2, 0);

    ast->gencode(compiler, 0);

    sljit_emit_return(compiler, SLJIT_MOV, SLJIT_R0, 0);

    code = sljit_generate_code(compiler);
    length = sljit_get_generated_code_size(compiler);

    return JITCode{code, length};
}

ComparatorExpression::~ComparatorExpression() { }

JITCode ComparatorExpression::compile() {
    using namespace mathexpr;
    switch (_comparator) {
    case ComparatorType::EQ_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "="_opr, std::move(_expr1)));
    case ComparatorType::NE_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "!="_opr, std::move(_expr1)));
    case ComparatorType::GT_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), ">"_opr, std::move(_expr1)));
    case ComparatorType::GE_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), ">="_opr, std::move(_expr1)));
    case ComparatorType::LT_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "<"_opr, std::move(_expr1)));
    case ComparatorType::LE_Expr:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "<="_opr, std::move(_expr1)));
    case ComparatorType::EQ_Range:
        return compile_ast(std::make_unique<ASTRange>(std::make_unique<ASTRef>("$new"), std::move(_expr1), std::move(_expr2)));
    case ComparatorType::NE_Range:
        return compile_ast(std::make_unique<ASTRange>(std::make_unique<ASTRef>("$new"), std::move(_expr1), std::move(_expr2), true));
    case ComparatorType::EQ_Mask:
        return compile_ast(std::make_unique<ASTMask>(std::make_unique<ASTRef>("$new"), std::move(_expr1), std::move(_expr2)));
    case ComparatorType::NE_Mask:
        return compile_ast(std::make_unique<ASTMask>(std::make_unique<ASTRef>("$new"), std::move(_expr1), std::move(_expr2), true));
    case ComparatorType::Boolean:
        return compile_ast(std::move(_expr1));
    case ComparatorType::EQ:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "="_opr, std::make_unique<ASTRef>("$old")));
    case ComparatorType::NE:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "!="_opr, std::make_unique<ASTRef>("$old")));
    case ComparatorType::GT:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), ">"_opr, std::make_unique<ASTRef>("$old")));
    case ComparatorType::GE:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), ">="_opr, std::make_unique<ASTRef>("$old")));
    case ComparatorType::LT:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "<"_opr, std::make_unique<ASTRef>("$old")));
    case ComparatorType::LE:
        return compile_ast(std::make_unique<ASTOpr2>(std::make_unique<ASTRef>("$new"), "<="_opr, std::make_unique<ASTRef>("$old")));
    default:
        assert(false && "Unsupported comparator");
    }
    ::abort();
}

void JITCode::free_code() {
    if (_code) {
        sljit_free_code(_code, nullptr);
        _code = nullptr;
        _length = 0;
    }
}

JITCode compile_math_expression(const std::string& string) {
    return compile_ast(mathexpr::parse(string));
}

ComparatorExpression parse_comparator_expression(const std::string& string)
{
    using namespace compexpr;
    struct Context { };
    std::istringstream iss{string};
    Tokenizer tokenizer{"<commandline>", iss, std::cout};
    Parser<Context> parser{};
    Context context{};
    auto comparator = parser.parse(context, tokenizer);
    
    if (comparator._expr1) {
        auto* num = dynamic_cast<mathexpr::ASTNumber*>(comparator._expr1.get());
        if (num) {
            comparator._constant1 = num->_value;
        }
    }

    if (comparator._expr2) {
        auto* num = dynamic_cast<mathexpr::ASTNumber*>(comparator._expr2.get());
        if (num) {
            comparator._constant2 = num->_value;
        }
    }

    return comparator;
}

std::pair<std::string, std::vector<std::string>> parse_command(const std::string& string) {
    try {
        return cmd::parse(string);
    } catch(...) {
        return {{},{}};
    }
}

} // namespace dsl
