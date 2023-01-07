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
#ifndef __compexpr_hpp__
#define __compexpr_hpp__

#include "mathexpr.hpp"
#include "dsl.hpp"

namespace compexpr {
using namespace mathexpr;
using namespace dsl;
using namespace playlang;

struct COMPARATOR : public Symbol<ComparatorExpression> {
    COMPARATOR(EQ&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::EQ_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(NE&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::NE_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(GT&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::GT_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(GE&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::GE_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(LT&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::LT_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(LE&, EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::LE_Expr;
        this->value()._expr1 = expr.release();
    };
    COMPARATOR(EQ&, LSQ&, EXPR& min, COMMA&, EXPR& max, RSQ&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::EQ_Range;
        this->value()._expr1 = min.release();
        this->value()._expr2 = max.release();
    };
    COMPARATOR(EQ&, LBK&, EXPR& value, COMMA&, EXPR& mask, RBK&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::EQ_Mask;
        this->value()._expr1 = value.release();
        this->value()._expr2 = mask.release();
        
    };
    COMPARATOR(NE&, LSQ&, EXPR& min, COMMA&, EXPR& max, RSQ&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::NE_Range;
        this->value()._expr1 = min.release();
        this->value()._expr2 = max.release();
    };
    COMPARATOR(NE&, LBK&, EXPR& value, COMMA&, EXPR& mask, RBK&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::NE_Mask;
        this->value()._expr1 = value.release();
        this->value()._expr2 = mask.release();
        
    };
    COMPARATOR(EQ&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::EQ;
    };
    COMPARATOR(NE&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::NE;
    };
    COMPARATOR(GT&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::GT;
    };
    COMPARATOR(GE&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::GE;
    };
    COMPARATOR(LT&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::LT;
    };
    COMPARATOR(LE&)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::LE;
    };
    COMPARATOR(EXPR& expr)
    :Symbol<ComparatorExpression>({})
    {
        this->value()._comparator = ComparatorType::Boolean;
        this->value()._expr1 = expr.release();
    };
};

} // namespace compexpr

#endif
