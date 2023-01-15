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
#ifndef __process_hpp__
#define __process_hpp__

#include <sys/uio.h>

#include "vmmap.hpp"

namespace mypower {

class Process {
    pid_t _pid;

public:
    Process(pid_t pid)
        : _pid(pid)
    {
    }

    pid_t pid() const { return _pid; }

    ssize_t read(VMAddress address, void* buffer, size_t size);
    ssize_t write(VMAddress address, const void* buffer, size_t size);
    ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count);
    ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count);

    bool suspend(bool same_user = false);
    bool resume(bool same_user = false);
};

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

} // namespace mypower

#endif
