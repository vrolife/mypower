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

#include <sljitLir.h>


namespace dsl {

class ExprCode {
    void* _code;
    size_t _length;

public:
    ExprCode(void* code, size_t length)
    : _code(code), _length(length)
    { }
    ~ExprCode() {
        this->free_code();
    }
    ExprCode(const ExprCode&) = delete;
    ExprCode& operator=(const ExprCode&) = delete;
    ExprCode(ExprCode&& other) noexcept {
        _code = other._code;
        _length = other._length;
        other._code = nullptr;
        other._length = 0;
    }
    ExprCode& operator=(ExprCode&& other) noexcept {
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

ExprCode compile_expr(const std::string& string);

std::pair<std::string, std::vector<std::string>> parse_command(const std::string& string);

}

#endif
