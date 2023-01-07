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
#ifndef __test_common_hpp__
#define __test_common_hpp__

#include "dsl.hpp"

inline
uintptr_t compile_and_run(const std::string& code, uintptr_t old=0, uintptr_t _new=0, uintptr_t address=0) {
    auto exprcode = dsl::compile_math_expression(code);
    return exprcode(old, _new, address);
}

#endif
