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
    assert(compile_and_run("0x123+100") == (0x123 + 100));
    assert(compile_and_run("0x123-100") == (0x123 - 100));
    assert(compile_and_run("0x123*100") == (0x123 * 100));
    assert(compile_and_run("0x123/100") == (0x123 / 100));
    assert(compile_and_run("0x123%100") == (0x123 % 100));

    assert(compile_and_run("0x10+0x3*0o5") == 0x1F);
    assert(compile_and_run("(0x10+0x3)*0o5") == ((0x10+0x3)*05));

    assert(compile_and_run("1+3&1") == 0);
    assert(compile_and_run("2&4|8") == (2&4|8));
    assert(compile_and_run("~1+2") == 0);
    assert(compile_and_run("1+3<<1") == 8);
    assert(compile_and_run("2>1|2") == 3);
    assert(compile_and_run("1?2:3") == 2);
    assert(compile_and_run("0?2:3") == 3);

    assert(compile_and_run("((10-2)+0x3)*((4+5)+(5-2))") == 132);
    return 0;
}