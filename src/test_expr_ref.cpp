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

#include "testcommon.hpp"

using namespace expr;

int main(int argc, char *argv[])
{
    assert(compile_and_run("$old", 13, 12, 3) == 13);
    assert(compile_and_run("$new", 1, 2, 3) == 2);
    assert(compile_and_run("$addr", 1, 2, 3) == 3);
    assert(compile_and_run("$old+1", 99) == 100);
    return 0;
}