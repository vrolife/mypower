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

enum ProcessState {
    Running = 'R',
    Sleeping = 'S',
    DiskSleep = 'D',
    Zombie = 'Z',
    Stopped = 'T',
    TracingStop = 't',
    DeadX = 'X',
    Deadx = 'x',
    Wakekill = 'K',
    Waking = 'W',
    Parked = 'P',
};

struct Process {
    virtual pid_t pid() const = 0;

    virtual ssize_t read(VMAddress address, void* buffer, size_t size) = 0;
    virtual ssize_t write(VMAddress address, const void* buffer, size_t size) = 0;
    virtual ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) = 0;
    virtual ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) = 0;

    virtual bool suspend(bool same_user = false) = 0;
    virtual bool resume(bool same_user = false) = 0;

    virtual ProcessState get_process_state() = 0;

    virtual VMRegion::ListType get_memory_regions() = 0;
};

class ProcessLinux : public Process {
    pid_t _pid;

public:
    ProcessLinux(pid_t pid)
        : _pid(pid)
    {
    }

    pid_t pid() const override { return _pid; }

    ssize_t read(VMAddress address, void* buffer, size_t size) override;
    ssize_t write(VMAddress address, const void* buffer, size_t size) override;
    ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override;
    ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override;

    bool suspend(bool same_user = false) override;
    bool resume(bool same_user = false) override;

    ProcessState get_process_state() override;

    VMRegion::ListType get_memory_regions() override;
};

class AutoSuspendResume {
    std::shared_ptr<Process> _process;
    bool _same_user;

public:
    AutoSuspendResume(std::shared_ptr<Process>& process, bool same_user = false, bool enable = true)
        : _process(process)
        , _same_user(same_user)
    {
        if (_process->get_process_state() != Running) {
            enable = false;
        }

        if (not enable) {
            _process = nullptr;
        }

        if (_process) {
            _process->suspend(same_user);
        }
    }

    ~AutoSuspendResume()
    {
        if (_process) {
            _process->resume(_same_user);
        }
    }
};

std::string read_process_comm(pid_t);
std::string read_process_cmdline(pid_t);

template <typename F>
void for_each_process(F&& callback)
{
    auto procfs = std::filesystem::path("/proc");

    auto begin = std::filesystem::directory_iterator(procfs);
    auto end = std::filesystem::directory_iterator();
    for (auto iter = begin; iter != end; ++iter) {
        pid_t pid = -1;
        try {
            pid = std::stoi(iter->path().stem());
        } catch (...) {
        }
        if (pid != -1) {
            callback(pid);
        }
    }
}

} // namespace mypower

#endif
