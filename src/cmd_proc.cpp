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
#include <regex>

#include <boost/program_options.hpp>

#include "mypower.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

class ProcessListView : public VisibleContainer<std::pair<pid_t, std::string>> {
    std::string _filter {};

public:
    ProcessListView()
    {
        refresh();
    }

    ProcessListView(const std::string& filter)
        : _filter { filter }
    {
        refresh();
    }

    AttributedString tui_title(size_t width)
    {
        return AttributedString::layout("Process", width, 1, '=', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width)
    {
        using namespace std::string_literals;
        using namespace tui::attributes;

        if (index >= this->size()) {
            return AttributedString {};
        }

        auto& data = this->at(index);

        AttributedStringBuilder builder {};
        builder << data.first << '\t' << data.second;

        return builder.release();
    }

    std::string tui_select(size_t index)
    {
        if (index >= this->size()) {
            return {};
        }
        auto& data = this->at(index);
        std::ostringstream command {};
        command << "attach " << data.first << std::endl;
        return command.str();
    }

    void refresh()
    {
        std::regex regexp { _filter };

        clear();
        for_each_process([&](pid_t pid) {
            auto comm = read_process_comm(pid);
            if (not _filter.empty()) {
                if (not std::regex_match(comm, regexp)) {
                    return;
                }
            }
            this->emplace_back(std::pair<pid_t, std::string> { pid, std::move(comm) });
        });
        tui_notify_changed();
    }
};

class CommandProcess : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandProcess(Application& app)
        : Command(app, "proc")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("filter,f", po::value<std::string>(), "regex filter");
        _posiginal.add("filter", 1);
    }

    std::string complete(const std::string& input) override
    {
        if ("ps"s.find(input) == 0) {
            return "ps";
        }
        if ("findps"s.find(input) == 0) {
            return "findps";
        }
        if ("findpsex"s.find(input) == 0) {
            return "findpsex";
        }
        return {};
    }

    bool match(const std::string& command) override
    {
        return command == "ps" or command == "findps" or command == "findpsex";
    }

    void show_short_help() override
    {
        message() << "ps\t\t\tList all processes";
        message() << "findps\t\t\tList processes contains substring";
        message() << "findpsex\t\tList processes matches regex";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        if (command == "ps") {
            show(std::make_shared<ProcessListView>());
            return;
        }

        PROGRAM_OPTIONS();

        std::string filter {};

        try {
            if (opts.count("filter")) {
                filter = opts["filter"].as<std::string>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        if (filter.empty() or opts.count("help")) {
            message() << "Usage: " << command << " [options] regex\n"
                      << _options;
            show();
            return;
        }

        if (command == "findps") {
            filter = ".*"s + filter + ".*"s;
        }

        show(std::make_shared<ProcessListView>(filter));
    }
};

static RegisterCommand<CommandProcess> _ListProcess {};

} // namespace mypower
