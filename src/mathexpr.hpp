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
#ifndef __mathexpr_hpp__
#define __mathexpr_hpp__

#include <memory>
#include <unordered_map>

#include <sljitLir.h>

#include "playlang/playlang.hpp"

namespace mathexpr {

using namespace playlang;

#define EXPR_VOID_TOKEN(n) \
struct n : public Token<void> { \
    template<typename Tokenizer> \
    explicit n(Tokenizer& tok) { } \
    explicit n() { } \
};

EXPR_VOID_TOKEN(PLUS)
EXPR_VOID_TOKEN(TIMES)
EXPR_VOID_TOKEN(DIVIDE)
EXPR_VOID_TOKEN(AND)
EXPR_VOID_TOKEN(OR)
EXPR_VOID_TOKEN(XOR)
EXPR_VOID_TOKEN(NOT)
EXPR_VOID_TOKEN(LAND)
EXPR_VOID_TOKEN(LOR)
EXPR_VOID_TOKEN(LNOT)
EXPR_VOID_TOKEN(GT)
EXPR_VOID_TOKEN(GE)
EXPR_VOID_TOKEN(LT)
EXPR_VOID_TOKEN(LE)
EXPR_VOID_TOKEN(EQ)
EXPR_VOID_TOKEN(NE)
EXPR_VOID_TOKEN(MOD)
EXPR_VOID_TOKEN(LSHIFT)
EXPR_VOID_TOKEN(RSHIFT)
EXPR_VOID_TOKEN(MINUS)
EXPR_VOID_TOKEN(LPAR)
EXPR_VOID_TOKEN(RPAR)
EXPR_VOID_TOKEN(UMINUS)
EXPR_VOID_TOKEN(EQUALS)
EXPR_VOID_TOKEN(QUESTION)
EXPR_VOID_TOKEN(COLON)
EXPR_VOID_TOKEN(LSQ)
EXPR_VOID_TOKEN(RSQ)
EXPR_VOID_TOKEN(LBK)
EXPR_VOID_TOKEN(RBK)
EXPR_VOID_TOKEN(COMMA)
EXPR_VOID_TOKEN(MISMATCH)

struct REFERENCE : public Token<std::string> {
    template<typename Tokenizer>
    REFERENCE(Tokenizer& tok): Token<std::string>(tok) { }
};
struct HEX : public Token<std::string> {
    template<typename Tokenizer>
    HEX(Tokenizer& tok): Token<std::string>(tok) { }
};
struct OCT : public Token<std::string> {
    template<typename Tokenizer>
    OCT(Tokenizer& tok): Token<std::string>(tok) { }
};
struct BIN : public Token<std::string> {
    template<typename Tokenizer>
    BIN(Tokenizer& tok): Token<std::string>(tok) { }
};
struct INTEGER : public Token<std::string> {
    template<typename Tokenizer>
    INTEGER(Tokenizer& tok): Token<std::string>(tok) { }
};

struct NUMBER : public Symbol<uintptr_t> {
    NUMBER(BIN& num)
    :Symbol<uintptr_t>(std::stoul(num.value().c_str() + 2, nullptr, 2))
    { };
    NUMBER(HEX& num)
    :Symbol<uintptr_t>(std::stoul(num.value().c_str() + 2, nullptr, 16))
    { };
    NUMBER(OCT& num)
    :Symbol<uintptr_t>(std::stoul(num.value().c_str() + 2, nullptr, 8))
    { };
    NUMBER(INTEGER& num)
    :Symbol<uintptr_t>(std::stoul(num.value(), nullptr, 10))
    { };
};

struct Compiler {
    size_t _local_size{0};
    std::unordered_map<std::string, uintptr_t> _local_vars{};
    size_t _reference_count{0};

    struct sljit_compiler* _compiler{nullptr};

    Compiler() {
        _compiler = sljit_create_compiler(this, this);
        assert(_compiler);
    }

    ~Compiler() {
        if (_compiler) {
            sljit_free_compiler(_compiler);
            _compiler = nullptr;
        }
    }

    uintptr_t reference(const std::string& name) {
        auto iter = _local_vars.find(name);
        if (iter == _local_vars.end()) {
            throw SyntaxError(std::string("Unknown variable: ") + name);
        }
        _reference_count += 1;
        return iter->second;
    }

    operator struct sljit_compiler*() {
        return _compiler;
    }
};

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual size_t depth(size_t depth) = 0;
    virtual void gencode(Compiler& compiler, size_t depth) = 0;
};

struct ASTRetarget : ASTNode {
    sljit_s32 _target{SLJIT_R0};

};

struct ASTNumber : ASTRetarget {
    uintptr_t _value;
    ASTNumber(uintptr_t value): _value(value) { }
    
    size_t depth(size_t depth) override {
        return depth;
    }

    void gencode(Compiler& compiler, size_t depth) override {
        sljit_emit_op1(compiler, SLJIT_MOV, _target, 0, SLJIT_IMM, _value);
    }
};

struct ASTRef : ASTRetarget {
    std::string _name;

    ASTRef(std::string&& name)
    : _name(std::move(name)) { }

    size_t depth(size_t depth) override {
        return depth;
    }
    
    void gencode(Compiler& compiler, size_t depth) override {
        auto offset = compiler.reference(_name);
        sljit_emit_op1(compiler, SLJIT_MOV, _target, 0, SLJIT_MEM1(SLJIT_SP), offset);
    }
};

template<typename T>
constexpr
T hash_string(char const* s, T value) noexcept
{
    return (*s != 0) ? hash_string(s+1, (value << 5) + value + *s ) : value;
}

// Fxck android wchar_t
constexpr static uintptr_t operator"" _opr(char const* s, size_t n) noexcept {
    return hash_string(s, 5381);
}

struct ASTOpr2 : ASTNode {
    size_t _opr;
    std::unique_ptr<ASTNode> _lexpr;
    std::unique_ptr<ASTNode> _rexpr;

    ASTOpr2(std::unique_ptr<ASTNode>&& l, size_t opr, std::unique_ptr<ASTNode>&& r)
    : _lexpr(std::move(l)), _opr(opr), _rexpr(std::move(r)) { }
    
    size_t depth(size_t depth) override {
        return std::max(_lexpr->depth(depth+1), _rexpr->depth(depth));
    }

    void gencode(Compiler& compiler, size_t depth) override 
    {
        ASTRetarget* retarget1 = dynamic_cast<ASTRetarget*>(_lexpr.get());
        ASTRetarget* retarget2 = dynamic_cast<ASTRetarget*>(_rexpr.get());
        if (retarget2) {
            retarget2->_target = SLJIT_R1;
            _lexpr->gencode(compiler, depth);
            _rexpr->gencode(compiler, depth);
        } else if (retarget1) {
            _rexpr->gencode(compiler, depth);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_R0, 0);
            _lexpr->gencode(compiler, depth);
        } else {
            auto offset = (depth + compiler._local_size) * sizeof(uintptr_t);
            _rexpr->gencode(compiler, depth);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset, SLJIT_R0, 0);
            _lexpr->gencode(compiler, depth + 1);
            sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_SP), offset);
        }

        switch(_opr) {
            case "/"_opr:
                sljit_emit_op0(compiler, SLJIT_DIV_UW);
                break;
            case "%"_opr:
                sljit_emit_op0(compiler, SLJIT_DIVMOD_UW);
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "*"_opr:
                sljit_emit_op0(compiler, SLJIT_LMUL_UW);
                break;
            case "+"_opr:
                sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "-"_opr:
                sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "&"_opr:
                sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "|"_opr:
                sljit_emit_op2(compiler, SLJIT_OR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "<<"_opr:
                sljit_emit_op2(compiler, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case ">>"_opr:
                sljit_emit_op2(compiler, SLJIT_LSHR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
                break;
            case "&&"_opr: {
                // if r0 == 0 then goto zero;
                auto* zero = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
                // if r1 == 0 then goto zero;
                auto* zero2 = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
                // r0 = 1
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 1);
                auto* one = sljit_emit_jump(compiler, SLJIT_JUMP);
                // zero:
                sljit_set_label(zero, sljit_emit_label(compiler));
                sljit_set_label(zero2, sljit_emit_label(compiler));
                // r0 = 0
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
                // one:
                sljit_set_label(one, sljit_emit_label(compiler));
                break;
            }
            case "||"_opr: {
                // if r0 != 0 then goto one;
                auto* one = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
                // if r1 == 0 then goto zero;
                auto* zero = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R1, 0, SLJIT_IMM, 0);
                // one:
                sljit_set_label(one, sljit_emit_label(compiler));
                // r0 = 1
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 1);
                // goto out
                auto* out = sljit_emit_jump(compiler, SLJIT_JUMP);
                // zero:
                sljit_set_label(zero, sljit_emit_label(compiler));
                // r0 = 0
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
                // out:
                sljit_set_label(out, sljit_emit_label(compiler));
                break;
            }
            case ">"_opr: 
            case ">="_opr: 
            case "<"_opr: 
            case "<="_opr: 
            case "="_opr: 
            case "!="_opr: 
            {
                int op;
                switch(_opr) {
                    case ">"_opr: op = SLJIT_GREATER; break;
                    case ">="_opr: op = SLJIT_GREATER_EQUAL; break;
                    case "<"_opr: op = SLJIT_LESS; break;
                    case "<="_opr: op = SLJIT_LESS_EQUAL; break;
                    case "="_opr: op = SLJIT_EQUAL; break;
                    case "!="_opr: op = SLJIT_NOT_EQUAL; break;
                    default: break;
                }
                // r0 opr r1
                auto* one = sljit_emit_cmp(compiler, op, SLJIT_R0, 0, SLJIT_R1, 0);
                // r0 = 0
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
                // goto out
                auto* out = sljit_emit_jump(compiler, SLJIT_JUMP);
                // one:
                sljit_set_label(one, sljit_emit_label(compiler));
                // r0 = 1
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 1);
                // out:
                sljit_set_label(out, sljit_emit_label(compiler));
                break;
            }
            default:
                assert(false && "Unsupported operator");
        }
    }
};

struct ASTOpr1 : ASTNode {
    size_t _opr;
    std::unique_ptr<ASTNode> _expr;

    ASTOpr1(size_t opr, std::unique_ptr<ASTNode>&& r)
    : _opr(opr), _expr(std::move(r)) { }

    size_t depth(size_t depth) override {
        return _expr->depth(depth + 1);
    }
    
    void gencode(Compiler& compiler, size_t depth) override {
        _expr->gencode(compiler, depth);
        switch(_opr) {
            case "-"_opr:
                sljit_emit_op2(compiler, SLJIT_SUB, SLJIT_R0, 0, SLJIT_IMM, 0, SLJIT_R0, 0);
                break;
            case "~"_opr:
                sljit_emit_op2(compiler, SLJIT_XOR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (sljit_sw)-1);
                break;
            case "!"_opr: {
                // if r0 == 0 then goto zero;
                auto* zero = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
                // r0 = 1
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 1);
                auto* one = sljit_emit_jump(compiler, SLJIT_JUMP);
                // zero:
                sljit_set_label(zero, sljit_emit_label(compiler));
                // r0 = 0
                sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
                // one:
                sljit_set_label(one, sljit_emit_label(compiler));
                break;
            }
        }
    }
};

struct ASTOpr3 : ASTNode {
    std::unique_ptr<ASTNode> _cond;
    std::unique_ptr<ASTNode> _expr1;
    std::unique_ptr<ASTNode> _expr2;

    ASTOpr3(std::unique_ptr<ASTNode>&& cond, std::unique_ptr<ASTNode>&& expr1, std::unique_ptr<ASTNode>&& expr2)
    : _cond(std::move(cond)), _expr1(std::move(expr1)), _expr2(std::move(expr2)) { }
    
    size_t depth(size_t depth) override {
        return std::max(_cond->depth(depth), std::max(_expr1->depth(depth), _expr2->depth(depth)));
    }

    void gencode(Compiler& compiler, size_t depth) override {
        _cond->gencode(compiler, depth);
        // if r0 == 0 then goto zero;
        auto* zero = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 0);
        _expr1->gencode(compiler, depth);
        auto* out = sljit_emit_jump(compiler, SLJIT_JUMP);
        // zero:
        sljit_set_label(zero, sljit_emit_label(compiler));
        _expr2->gencode(compiler, depth);
        // out:
        sljit_set_label(out, sljit_emit_label(compiler));
    }
};

struct ASTRange : ASTNode {
    std::unique_ptr<ASTNode> _expr;
    std::unique_ptr<ASTNode> _min;
    std::unique_ptr<ASTNode> _max;
    bool _reverse;

    ASTRange(std::unique_ptr<ASTNode>&& expr, std::unique_ptr<ASTNode>&& min, std::unique_ptr<ASTNode>&& max, bool reverse=false)
    : _expr(std::move(expr))
    , _min(std::move(min))
    , _max(std::move(max))
    , _reverse(!!reverse)
    { }

    size_t depth(size_t depth) override {
        return std::max(_expr->depth(depth), std::max(_min->depth(depth+1), _max->depth(depth+2)));
    }

    void gencode(Compiler& compiler, size_t depth) override {
        auto offset = (depth + compiler._local_size) * sizeof(uintptr_t);
        _expr->gencode(compiler, depth);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset, SLJIT_R0, 0);
        _min->gencode(compiler, depth + 1);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset + sizeof(uintptr_t), SLJIT_R0, 0);
        _max->gencode(compiler, depth + 2);

        // R1 = expr
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_SP), offset);
        // if R1 > R0(max) then goto oor
        auto* oor = sljit_emit_cmp(compiler, SLJIT_GREATER, SLJIT_R1, 0, SLJIT_R0, 0);
        // R0 = min
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM1(SLJIT_SP), offset + sizeof(uintptr_t));
        // if R1 < R0(min) then goto oor2
        auto* oor2 = sljit_emit_cmp(compiler, SLJIT_LESS, SLJIT_R1, 0, SLJIT_R0, 0);
        // R0 = 1
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, not _reverse);
        // goto out
        auto* out = sljit_emit_jump(compiler, SLJIT_JUMP);

        sljit_set_label(oor, sljit_emit_label(compiler));
        sljit_set_label(oor2, sljit_emit_label(compiler));

        // R0 = 0
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, _reverse);

        sljit_set_label(out, sljit_emit_label(compiler));
    }
};

struct ASTMask : ASTNode {
    std::unique_ptr<ASTNode> _expr;
    std::unique_ptr<ASTNode> _value;
    std::unique_ptr<ASTNode> _mask;
    bool _reverse;

    ASTMask(std::unique_ptr<ASTNode>&& expr, std::unique_ptr<ASTNode>&& value, std::unique_ptr<ASTNode>&& mask, bool reverse=false)
    : _expr(std::move(expr))
    , _value(std::move(value))
    , _mask(std::move(mask))
    , _reverse(!!reverse)
    { }

    size_t depth(size_t depth) override {
        return std::max(_value->depth(depth), std::max(_mask->depth(depth+1), _expr->depth(depth+2)));
    }

    void gencode(Compiler& compiler, size_t depth) override {
        auto offset = (depth + compiler._local_size) * sizeof(uintptr_t);
        _value->gencode(compiler, depth);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset, SLJIT_R0, 0);
        _mask->gencode(compiler, depth + 1);
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), offset + sizeof(uintptr_t), SLJIT_R0, 0);

        // R0 = expr
        _expr->gencode(compiler, depth + 2);
        // R1 = value
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_SP), offset);
        // R2 = mask
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_MEM1(SLJIT_SP), offset + sizeof(uintptr_t));
        // R0 &= mask
        sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        // R1 &= mask
        sljit_emit_op2(compiler, SLJIT_AND, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);

        // if R0(expr) != R1(max) then goto ne
        auto* ne = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, SLJIT_R0, 0, SLJIT_R1, 0);
        // R0 = 1
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, not _reverse);
        // goto out
        auto* out = sljit_emit_jump(compiler, SLJIT_JUMP);

        sljit_set_label(ne, sljit_emit_label(compiler));
        // R0 = 0
        sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, _reverse);

        sljit_set_label(out, sljit_emit_label(compiler));
    }
};

struct EXPR : public Symbol<std::unique_ptr<ASTNode>> 
{
    static bool _constant_optimal;

    void opt1() {
        if (not _constant_optimal) {
            return;
        }
        auto* opr1 = dynamic_cast<ASTOpr1*>(value().get());
        assert(opr1);
        auto* num = dynamic_cast<ASTNumber*>(opr1->_expr.get());
        if (num == nullptr) {
            return;
        }

        switch(opr1->_opr) {
            case "-"_opr:
                value() = std::make_unique<ASTNumber>(-num->_value);
                break;
            case "~"_opr:
                value() = std::make_unique<ASTNumber>(~num->_value);
                break;
            case "!"_opr: {
                value() = std::make_unique<ASTNumber>(!num->_value);
                break;
            }
        }
    }

    void opt2() {
        if (not _constant_optimal) {
            return;
        }
        auto* opr2 = dynamic_cast<ASTOpr2*>(value().get());
        assert(opr2);
        auto* num1 = dynamic_cast<ASTNumber*>(opr2->_lexpr.get());
        auto* num2 = dynamic_cast<ASTNumber*>(opr2->_rexpr.get());
        if (num1 == nullptr or num2 == nullptr) {
            return;
        }

        switch(opr2->_opr) {
            case "/"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value / num2->_value);
                break;
            case "%"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value % num2->_value);
                break;
            case "*"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value * num2->_value);
                break;
            case "+"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value + num2->_value);
                break;
            case "-"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value - num2->_value);
                break;
            case "&"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value & num2->_value);
                break;
            case "|"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value | num2->_value);
                break;
            case "<<"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value << num2->_value);
                break;
            case ">>"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value >> num2->_value);
                break;
            case "&&"_opr:
                value() = std::make_unique<ASTNumber>(num1->_value && num2->_value);
                break;
            case "||"_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value || num2->_value);
                break;
            case ">"_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value > num2->_value);
                break;
            case ">="_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value >= num2->_value);
                break;
            case "<"_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value < num2->_value);
                break;
            case "<="_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value <= num2->_value);
                break;
            case "="_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value == num2->_value);
                break;
            case "!="_opr: 
                value() = std::make_unique<ASTNumber>(num1->_value != num2->_value);
                break;
        }
    }

    void opt3() {
        if (not _constant_optimal) {
            return;
        }
        auto* opr3 = dynamic_cast<ASTOpr3*>(value().get());
        assert(opr3);
        auto* cond = dynamic_cast<ASTNumber*>(opr3->_cond.get());
        if (cond == nullptr) {
            return;
        }
        value() = cond->_value ? std::move(opr3->_expr1) : std::move(opr3->_expr2);
    }

    EXPR(EXPR& expr1, DIVIDE&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(), "/"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, MOD&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "%"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, TIMES&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "*"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, PLUS&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "+"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, MINUS&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "-"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, AND&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "&"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, OR&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "|"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, XOR&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "^"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, LAND&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "&&"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, LOR&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "||"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, GT&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  ">"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, GE&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  ">="_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, LT&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "<"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, LE&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "<="_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, EQ&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "="_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, NE&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "!="_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, LSHIFT&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  "<<"_opr, expr2.release()))
    { opt2(); };
    EXPR(EXPR& expr1, RSHIFT&, EXPR& expr2)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr2>(expr1.release(),  ">>"_opr, expr2.release()))
    { opt2(); };
    EXPR(LPAR&, EXPR& expr, RPAR&)
    :Symbol<std::unique_ptr<ASTNode>>(expr.release())
    { };
    EXPR(MINUS&, EXPR& expr)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr1>("-"_opr, expr.release()))
    { opt1(); };
    EXPR(NOT&, EXPR& expr)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr1>("~"_opr, expr.release()))
    { opt1(); };
    EXPR(LNOT&, EXPR& expr)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr1>("!"_opr, expr.release()))
    { opt1(); };
    EXPR(REFERENCE& ref)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTRef>(ref.release()))
    { };
    EXPR(NUMBER& num)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTNumber>(num.value()))
    { };
    EXPR(EXPR& expr1, QUESTION&, EXPR& expr2, COLON&, EXPR& expr3)
    :Symbol<std::unique_ptr<ASTNode>>(std::make_unique<ASTOpr3>(expr1.release(), expr2.release(), expr3.release()))
    { opt3(); };
};

std::unique_ptr<ASTNode> parse(const std::string& str);

} // namespace mathexpr

#endif
