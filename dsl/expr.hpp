#ifndef __expr_hpp__
#define __expr_hpp__

#include "playlang/playlang.hpp"

#ifndef EXPR_TYPE
#define EXPR_TYPE uintptr_t
#endif

namespace expr {

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

struct NUMBER : public Symbol<EXPR_TYPE> {
    NUMBER(BIN& num)
    :Symbol<EXPR_TYPE>(std::stoul(num.value().c_str() + 2, nullptr, 2))
    { };
    NUMBER(HEX& num)
    :Symbol<EXPR_TYPE>(std::stoul(num.value().c_str() + 2, nullptr, 16))
    { };
    NUMBER(OCT& num)
    :Symbol<EXPR_TYPE>(std::stoul(num.value().c_str() + 2, nullptr, 8))
    { };
    NUMBER(INTEGER& num)
    :Symbol<EXPR_TYPE>(std::stoul(num.value(), nullptr, 10))
    { };
};

struct EXPR : public Symbol<EXPR_TYPE> {
    EXPR(EXPR& expr1, DIVIDE&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() / expr2.value())
    { };
    EXPR(EXPR& expr1, MOD&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() % expr2.value())
    { };
    EXPR(EXPR& expr1, TIMES&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() * expr2.value())
    { };
    EXPR(EXPR& expr1, PLUS&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() + expr2.value())
    { };
    EXPR(EXPR& expr1, MINUS&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() - expr2.value())
    { };
    EXPR(EXPR& expr1, AND&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() & expr2.value())
    { };
    EXPR(EXPR& expr1, OR&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() | expr2.value())
    { };
    EXPR(EXPR& expr1, XOR&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() ^ expr2.value())
    { };
    EXPR(EXPR& expr1, LAND&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() && expr2.value())
    { };
    EXPR(EXPR& expr1, LOR&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() || expr2.value())
    { };
    EXPR(EXPR& expr1, GT&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() > expr2.value())
    { };
    EXPR(EXPR& expr1, GE&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() >= expr2.value())
    { };
    EXPR(EXPR& expr1, LT&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() < expr2.value())
    { };
    EXPR(EXPR& expr1, LE&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() <= expr2.value())
    { };
    EXPR(EXPR& expr1, EQ&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() == expr2.value())
    { };
    EXPR(EXPR& expr1, NE&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() != expr2.value())
    { };
    EXPR(EXPR& expr1, LSHIFT&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() << expr2.value())
    { };
    EXPR(EXPR& expr1, RSHIFT&, EXPR& expr2)
    :Symbol<EXPR_TYPE>(expr1.value() >> expr2.value())
    { };
    EXPR(LPAR&, EXPR& expr, RPAR&)
    :Symbol<EXPR_TYPE>(expr.value())
    { };
    EXPR(MINUS&, EXPR& expr)
    :Symbol<EXPR_TYPE>(-expr.value())
    { };
    EXPR(NOT&, EXPR& expr)
    :Symbol<EXPR_TYPE>(~expr.value())
    { };
    EXPR(LNOT&, EXPR& expr)
    :Symbol<EXPR_TYPE>(!expr.value())
    { };
    EXPR(REFERENCE& expr)
    :Symbol<EXPR_TYPE>(0)
    {
        assert(false);
    };
    EXPR(NUMBER& num)
    :Symbol<EXPR_TYPE>(num.value())
    {
    };
};

EXPR_TYPE parse(const std::string& str);

} // namespace expr

#endif
