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
#include <fcntl.h>
#include <boost/program_options.hpp>

#include "mypower.hpp"
#include "raii.hpp"

using namespace std::string_literals;

namespace mypower {

MYPOWER_RAII_SIMPLE_HANDLE(UniqueFD, int, -1, ::close);

class CommandUtils : public Command {
public:
    CommandUtils(Application& app)
        : Command(app, "utils")
    {
    }
    std::string complete(const std::string& input) override
    {
        if ("disable-inotify"s.find(input) == 0) {
            return "disable-inotify";
        }
        return {};
    }
    bool match(const std::string& command) override
    {
        return command == "disable-inotify";
    }

    void show_short_help() override {
        message() << "disable-inotify\t\tWrite 0 to /proc/sys/fs/inotify/max_user_{instances,watches}";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        if (command == "disable-inotify") {
            UniqueFD instances{open("/proc/sys/fs/inotify/max_user_instances", O_WRONLY)};
            if (not instances.valid()) {
                message() << attributes::SetColor(attributes::ColorError)
                    << "Unable to open /proc/sys/fs/inotify/max_user_instances: "
                    << strerror(errno);
                return;
            }
            if (::write(instances, "0", 1) != 1) {
                message() << attributes::SetColor(attributes::ColorError)
                    << "Write \"0\" to /proc/sys/fs/inotify/max_user_instances failed: "
                    << strerror(errno);
                return;
            }

            UniqueFD watches{open("/proc/sys/fs/inotify/max_user_watches", O_WRONLY)};
            if (not watches.valid()) {
                message() << attributes::SetColor(attributes::ColorError)
                    << "Unable to open /proc/sys/fs/inotify/max_user_watches: "
                    << strerror(errno);
                return;
            }
            if (::write(watches, "0", 1) != 1) {
                message() << attributes::SetColor(attributes::ColorError)
                    << "Write \"0\" to /proc/sys/fs/inotify/max_user_watches failed: "
                    << strerror(errno);
                return;
            }
        }
    }
};

static RegisterCommand<CommandUtils> _Test {};

} // namespace mypower
