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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <climits>
#include <regex>
#include <fstream>

#include "processlist.hpp"

namespace mypower {

std::string read_process_file(pid_t pid, const char* filename, size_t buffer_size = PATH_MAX) {
    auto cmdline = std::filesystem::path("/proc") / std::to_string(pid) / filename;
    std::string buffer{};
    buffer.resize(buffer_size);

    int fd = ::open(cmdline.c_str(), O_RDONLY);
    if (fd == -1) {
        return {};
    }

    auto size = ::read(fd, buffer.data(), buffer.size());
    if (size < 0) {
        ::close(fd);
        return {};
    }
    ::close(fd);

    buffer.resize(size);
    return buffer;
}

std::string read_process_comm(pid_t pid)
{
    auto buffer = read_process_file(pid, "comm");

    auto iter = std::find_if(buffer.begin(), buffer.end(), [&](char c) {
        return c == '\n' or c == '\0';
    });

    buffer.resize(iter - buffer.begin());
    return buffer;
}

std::string read_process_cmdline(pid_t pid) {
    auto buffer = read_process_file(pid, "cmdline");

    for (auto& ch : buffer) {
        if (ch == '\0') {
            ch = ' ';
        }
    }
    return buffer;
}

ProcessList::ProcessList() {
    refresh();
}

ProcessList::ProcessList(const std::string& filter)
:_filter{filter}
{
    refresh();
}

StyleString ProcessList::tui_title(size_t width)
{
    return StyleString::layout("Process", width, 1, '=', LayoutAlign::Center);
}

StyleString ProcessList::tui_item(size_t index, size_t width)
{
    using namespace std::string_literals;
    using namespace tui::style;

    if (index >= this->size()) {
        return StyleString{};
    }

    auto& data = this->at(index);
    
    StyleStringBuilder builder{};
    builder << data.first << '\t' << data.second;

    return builder.release();
}

std::string ProcessList::tui_select(size_t index) {
    if (index >= this->size()) {
        return {};
    }
    auto& data = this->at(index);
    std::ostringstream command{};
    command << "attach " << data.first << std::endl;
    return command.str();
}

void ProcessList::refresh()
{
    std::regex regexp{_filter, std::regex::grep};

    clear();
    for_each_process([&](pid_t pid) {
        auto comm = read_process_comm(pid);
        if (not _filter.empty()) {
            if (not std::regex_match(comm, regexp)) {
                return;
            }
        }
        this->emplace_back(std::pair<pid_t, std::string>{ pid, std::move(comm) });
    });
    tui_notify_changed();
}

} // namespace mypower
