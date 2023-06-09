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
#ifndef __cmdline_hpp__
#define __cmdline_hpp__

#include <string>
#include <vector>

#include "playlang/playlang.hpp"

namespace cmdline {
using namespace playlang;

#define CMD_VOID_TOKEN(n)             \
    struct n : public Token<void> {   \
        template <typename Tokenizer> \
        explicit n(Tokenizer& tok)    \
        {                             \
        }                             \
        explicit n()                  \
        {                             \
        }                             \
    };

CMD_VOID_TOKEN(BOUNDARY);

struct STRING : public Token<std::string> {
    static char unescape(char cha)
    {
        switch (cha) {
        case 'r':
            return '\r';
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        default:
            return cha;
        }
    }

    STRING()
        : Token<std::string>({})
    {
    }
};

template <typename StringType>
struct COMMON_STRING_ESCAPE : public Token<void, true> {
    template <typename Tokenizer>
    explicit COMMON_STRING_ESCAPE(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.value().template as<StringType>().value().push_back(STRING::unescape(tok.at(1)));
    }
};

template <typename StringType>
struct COMMON_STRING_CHAR : public Token<void, true> {
    template <typename Tokenizer>
    explicit COMMON_STRING_CHAR(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.value().template as<StringType>().value().append((std::string)tok);
    }
};

struct COMMON_STRING_END : public Token<void, true> {
    template <typename Tokenizer>
    explicit COMMON_STRING_END(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.leave();
    }
};

struct BARE_STRING_BEGIN_ESCAPE : public Token<void, true> {

    template <typename Tokenizer>
    explicit BARE_STRING_BEGIN_ESCAPE(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.enter(Tokenizer::bare_string, typename Tokenizer::ValueType { STRING {} });
        tok.value().template as<STRING>().value().push_back(tok.at(1));
    }
};

struct BARE_STRING_BEGIN_ANY : public Token<void, true> {

    template <typename Tokenizer>
    explicit BARE_STRING_BEGIN_ANY(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.enter(Tokenizer::bare_string, typename Tokenizer::ValueType { STRING {} });
        tok.value().template as<STRING>().value().append((std::string)tok);
    }
};

typedef COMMON_STRING_ESCAPE<STRING> BARE_STRING_ESCAPE;
typedef COMMON_STRING_CHAR<STRING> BARE_STRING_CHAR;
typedef COMMON_STRING_END BARE_STRING_END;
typedef COMMON_STRING_END BARE_STRING_BOUNDARY;

struct BARE_STRING_EOF : public Token<void, true> {
    template <typename Tokenizer>
    explicit BARE_STRING_EOF(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.leave();
    }
};

struct BARE_STRING_CHAR_LPAR : public Token<void, true> {
    template <typename Tokenizer>
    explicit BARE_STRING_CHAR_LPAR(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.value().template as<STRING>().value().append((std::string)tok);
        tok.leave();
    }
};

struct QUOTED_STRING_BEGIN : public Token<void, true> {
    template <typename Tokenizer>
    explicit QUOTED_STRING_BEGIN(Tokenizer& tok)
        : Token<void, true>()
    {
        tok.enter(Tokenizer::quoted_string, typename Tokenizer::ValueType { STRING {} });
    }
};

typedef COMMON_STRING_ESCAPE<STRING> QUOTED_STRING_ESCAPE;
typedef COMMON_STRING_CHAR<STRING> QUOTED_STRING_CHAR;
typedef COMMON_STRING_END QUOTED_STRING_END;

struct EQ_OR_BOUNDARY : public Symbol<void> {
    template <typename Tokenizer>
    explicit EQ_OR_BOUNDARY(Tokenizer& tok) { }
};

struct LIST : public Symbol<std::vector<std::string>> {
    LIST()
        : Symbol<std::vector<std::string>>({}) {};
    LIST(STRING& s)
        : Symbol<std::vector<std::string>>({})
    {
        this->value().emplace_back(s.release());
    };
    LIST(LIST& lst, STRING& s)
        : Symbol<std::vector<std::string>>(lst.release())
    {
        this->value().emplace_back(s.release());
    };
};

struct COMMAND : public Symbol<std::pair<std::string, std::vector<std::string>>> {
    COMMAND(STRING& cmd, LIST& lst)
        : Symbol<std::pair<std::string, std::vector<std::string>>>({ cmd.release(), lst.release() }) {};
};

std::pair<std::string, std::vector<std::string>> parse(const std::string& str);

} // namespace cmdline

#endif
