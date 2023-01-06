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
#include "expr.hpp"

namespace comparator {
using namespace expr;
using namespace playlang;

enum class Operator {
    None,
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE,
    Boolean,
    EQ_Range,
    EQ_Mask,
    NE_Range,
    NE_Mask,
};

struct Comparator {
    Operator _operator{Operator::None};
    std::unique_ptr<ASTNode> _expr1{};
    std::unique_ptr<ASTNode> _expr2{};

    Comparator() = default;
};

struct COMPARATOR : public Symbol<Comparator> {
    COMPARATOR(EQ&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::EQ;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(NE&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::NE;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(GT&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::GT;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(GE&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::GE;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(LT&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::LT;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(LE&, EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::LE;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(EQ&, LSQ&, EXPR& min, COMMA&, EXPR& max, RSQ&)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::EQ_Range;
        this->value()._expr1 = min.release();
        this->value()._expr2 = max.release();
    };
    COMPARATOR(EQ&, LBK&, EXPR& value, COMMA&, EXPR& mask, RBK&)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::EQ_Mask;
        this->value()._expr1 = value.release();
        this->value()._expr2 = mask.release();
        
    };
    COMPARATOR(NE&, LSQ&, EXPR& min, COMMA&, EXPR& max, RSQ&)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::EQ_Range;
        this->value()._expr1 = min.release();
        this->value()._expr2 = max.release();
    };
    COMPARATOR(NE&, LBK&, EXPR& value, COMMA&, EXPR& mask, RBK&)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::EQ_Mask;
        this->value()._expr1 = value.release();
        this->value()._expr2 = mask.release();
        
    };
    COMPARATOR(EXPR& expr)
    :Symbol<Comparator>({})
    {
        this->value()._operator = Operator::Boolean;
        this->value()._expr1 = expr.release();
    };
};

} // namespace comparator
