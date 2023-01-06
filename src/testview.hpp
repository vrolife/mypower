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
#ifndef __testview_hpp__
#define __testview_hpp__

#include <random>

#include "tui.hpp"

namespace mypower {
using namespace tui;

class TestView : public VisibleContainer<ssize_t> {
public:
    TestView()
    {
        std::random_device rd{};
        std::uniform_int_distribution<ssize_t> dist(0, 1000);
        emplace_back(dist(rd));
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("Test", width, 1, '-', LayoutAlign::Center);
    }

    StyleString tui_item(size_t index, size_t width) override
    {
        switch (index) {
        case 0:
            return StyleString { std::to_string(at(0)) + " (INT32)"};
        case 1:
            return StyleString { "+1" };
        case 2:
            return StyleString { "-1" };
        case 3:
            return StyleString { "+5" };
        case 4:
            return StyleString { "-5" };
        }
        return {};
    }

    std::string tui_select(size_t index) override {
        switch(index) {
            case 1:
                at(0) += 1;
                tui_notify_changed();
                break;
            case 2:
                at(0) -= 1;
                tui_notify_changed();
                break;
            case 3:
                at(0) += 5;
                tui_notify_changed();
                break;
            case 4:
                at(0) -= 5;
                tui_notify_changed();
                break;
        }
        return {};
    }
};


} // namespace mypower

#endif
