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
#ifndef __processlist_hpp__
#define __processlist_hpp__

#include <iomanip>
#include <string>
#include <sstream>
#include <filesystem>

#include "tui.hpp"

namespace mypower {
using namespace tui;

std::string read_process_comm(pid_t);
std::string read_process_cmdline(pid_t);

template<typename F>
void for_each_process(F&& callback) {
    auto procfs = std::filesystem::path("/proc");

    auto begin = std::filesystem::directory_iterator(procfs);
    auto end = std::filesystem::directory_iterator();
    for (auto iter = begin; iter != end; ++ iter) {
        pid_t pid = -1;
        try {
            pid = std::stoi(iter->path().stem());
        } catch(...) { }
        if (pid != -1) {
            callback(pid);
        }
    }
}

class ProcessList : public VisibleContainer<std::pair<pid_t, std::string>> {
    std::string _filter{};
public:
    ProcessList();
    ProcessList(const std::string& filter);

    StyleString tui_title(size_t width) override;
    
    StyleString tui_item(size_t index, size_t width) override;

    std::string tui_select(size_t index) override;

    void refresh();
};

} // namespace mypower

#endif
