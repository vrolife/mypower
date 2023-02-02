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
#include <boost/program_options.hpp>

#include "mypower.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

class CommandAttach : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandAttach(Application& app)
        : Command(app, "atttach")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("pid,p", po::value<pid_t>(), "target process pid");
        _posiginal.add("pid", 1);
    }

    std::string complete(const std::string& input) override
    {
        if ("selfattach"s.find(input) == 0) {
            return "selfattach";
        }
        if ("attach"s.find(input) == 0) {
            return "attach";
        }
        return {};
    }

    bool match(const std::string& command) override
    {
        return command == "selfattach" or command == "attach";
    }

    void attach(pid_t pid)
    {
        message() << "Attach process " << pid;
        show();

        _app._process = std::make_shared<ProcessLinux>(pid);
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        if (command == "selfattach") {
            attach(::getpid());
            return;
        }

        PROGRAM_OPTIONS();

        pid_t pid = -1;

        try {
            if (opts.count("pid")) {
                pid = opts["pid"].as<pid_t>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        if (pid == -1 or opts.count("help")) {
            message()
                << "Usage: attach [options] pid\n"
                << _options;
            show();
            return;
        }

        attach(pid);
    }
};

static RegisterCommand<CommandAttach> _Attach {};

} // namespace mypower
