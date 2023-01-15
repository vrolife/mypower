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
#include <sys/uio.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include <algorithm>

#include "process.hpp"

namespace mypower {
namespace fs = std::filesystem;

ssize_t Process::read(VMAddress address, void* buffer, size_t size)
{
    struct iovec local {
        .iov_base = buffer, .iov_len = size
    };
    struct iovec remote {
        .iov_base = reinterpret_cast<void*>(address.get()), .iov_len = size
    };
    return process_vm_readv(_pid, &local, 1, &remote, 1, 0);
}

ssize_t Process::write(VMAddress address, const void* buffer, size_t size)
{
    struct iovec local {
        .iov_base = const_cast<void*>(buffer), .iov_len = size
    };
    struct iovec remote {
        .iov_base = reinterpret_cast<void*>(address.get()), .iov_len = size
    };
    return process_vm_readv(_pid, &local, 1, &remote, 1, 0);
}

ssize_t Process::read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count)
{
    return process_vm_readv(_pid, local, local_count, remote, remote_count, 0);
}

ssize_t Process::write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count)
{
    return process_vm_writev(_pid, local, local_count, remote, remote_count, 0);
}

static std::tuple<int, int> get_process_user_group(pid_t pid)
{
    auto path = fs::path { "/proc" } / std::to_string(pid);

    struct stat buf {
        0
    };

    if (stat(path.c_str(), &buf) != 0) {
        return { -1, -1 };
    }
    return { buf.st_uid, buf.st_gid };
}

static void kill_same_user(pid_t pid, int signal)
{
    auto mypid = getpid();
    auto [uid, gid] = get_process_user_group(pid);
    auto begin = fs::directory_iterator { "/proc" };
    auto end = fs::directory_iterator {};

    for (auto iter = begin; iter != end; ++iter) {
        auto& path = iter->path();
        auto stem = path.stem().string();

        // not a pid
        if (not std::isdigit(stem[0])) {
            continue;
        }

        pid_t process = std::stoi(stem);

        // scanner or target process
        if (process == pid or process == mypid) {
            continue;
        }

        struct stat buf {
            0
        };
        if (stat(path.c_str(), &buf) != 0) {
            continue;
        }

        // different user
        if (uid != buf.st_uid) {
            continue;
        }

        ::kill(process, signal);
    }
}

bool Process::suspend(bool same_user)
{
    if (same_user) {
        kill_same_user(_pid, SIGSTOP);
    }
    // TODO suspend user's all process
    return ::kill(_pid, SIGSTOP) == 0;
}
bool Process::resume(bool same_user)
{
    if (same_user) {
        kill_same_user(_pid, SIGCONT);
    }
    return ::kill(_pid, SIGCONT) == 0;
}

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

} // namespace mypower