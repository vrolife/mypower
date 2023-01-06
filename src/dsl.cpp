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
#include "dsl.hpp"

#include "expr.hpp"
#include "cmd.hpp"

namespace dsl {

void ExprCode::free_code() {
    if (_code) {
        sljit_free_code(_code, nullptr);
        _code = nullptr;
        _length = 0;
    }
}

ExprCode compile_expr(const std::string& string) {
    auto ast = expr::parse(string);
    
    void* code = nullptr;
    size_t length = 0;

    expr::Compiler compiler{};

    compiler._local_size = 3;
    compiler._local_vars.emplace("$old", 0);
    compiler._local_vars.emplace("$new", 1 * sizeof(uintptr_t));
    compiler._local_vars.emplace("$addr", 2 * sizeof(uintptr_t));
    compiler._local_vars.emplace("$address", 2 * sizeof(uintptr_t));

    auto depth = ast->depth(0);

    sljit_emit_enter(compiler, 0, SLJIT_ARGS3(W, W, W, W), 2, 3, 0, 0, (compiler._local_size + depth) * sizeof(uintptr_t));
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 0, SLJIT_S0, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 1 * sizeof(uintptr_t), SLJIT_S1, 0);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 2 * sizeof(uintptr_t), SLJIT_S2, 0);

    ast->gencode(compiler, 0);

    sljit_emit_return(compiler, SLJIT_MOV, SLJIT_R0, 0);

    code = sljit_generate_code(compiler);
    length = sljit_get_generated_code_size(compiler);

    return ExprCode{code, length};
}

std::pair<std::string, std::vector<std::string>> parse_command(const std::string& string) {
    try {
        return cmd::parse(string);
    } catch(...) {
        return {{},{}};
    }
}
}
