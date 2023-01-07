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
#ifndef __dsl_hpp__
#define __dsl_hpp__

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace mathexpr {
class ASTNode;
} // namespace expr

namespace dsl {

enum class ComparatorType {
    EQ_Expr,
    NE_Expr,
    GT_Expr,
    GE_Expr,
    LT_Expr,
    LE_Expr,
    EQ_Range,
    EQ_Mask,
    NE_Range,
    NE_Mask,
    Boolean,
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE,
    None
};

class JITCode {
    void* _code;
    size_t _length;

public:
    JITCode(void* code, size_t length)
    : _code(code), _length(length)
    { }
    ~JITCode() {
        this->free_code();
    }
    JITCode(const JITCode&) = delete;
    JITCode& operator=(const JITCode&) = delete;
    JITCode(JITCode&& other) noexcept {
        _code = other._code;
        _length = other._length;
        other._code = nullptr;
        other._length = 0;
    }
    JITCode& operator=(JITCode&& other) noexcept {
        this->free_code();
        _code = other._code;
        _length = other._length;
        other._code = nullptr;
        other._length = 0;
        return *this;
    }

    uintptr_t operator()(uintptr_t old, uintptr_t _new, uintptr_t addr) {
        return ((uintptr_t(*)(uintptr_t, uintptr_t, uintptr_t))_code)(old, _new, addr);
    }

private:
    void free_code();
};

struct ComparatorExpression {
    ComparatorType _comparator{ComparatorType::None};

    std::unique_ptr<mathexpr::ASTNode> _expr1{};
    std::unique_ptr<mathexpr::ASTNode> _expr2{};

    std::optional<uintptr_t> _constant1{};
    std::optional<uintptr_t> _constant2{};

    ComparatorExpression() = default;
    ComparatorExpression(ComparatorExpression&&) noexcept = default;
    ComparatorExpression(const ComparatorExpression&) = delete;
    ComparatorExpression& operator=(const ComparatorExpression&) = delete;
    ComparatorExpression& operator=(ComparatorExpression&&) noexcept = default;
    ~ComparatorExpression();

    JITCode compile();
};

JITCode compile_math_expression(const std::string& string);

ComparatorExpression parse_comparator_expression(const std::string& string);

std::pair<std::string, std::vector<std::string>> parse_command(const std::string& string);

}

#endif
