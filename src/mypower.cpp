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
#define __USE_GNU 1
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <iostream>
#include <regex>
#include <stack>

#include "dsl.hpp"
#include "scanner.hpp"
#include "tui.hpp"

#include <boost/program_options.hpp>

#define PARSE_ARG(name)                                                                       \
    try {                                                                                     \
        po::store(po::command_line_parser(arguments)                                          \
                      .options(_options_##name)                                               \
                      .positional(_posiginal_##name)                                          \
                      .run(),                                                                 \
            vm);                                                                              \
        notify(vm);                                                                           \
    } catch (const std::exception& exc) {                                                     \
        _message->stream()                                                                    \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " " << exc.what();                                                             \
        return;                                                                               \
    } catch (...) {                                                                           \
        _message->stream()                                                                    \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " Unknown error";                                                              \
        return;                                                                               \
    }

namespace po = boost::program_options;

using namespace mypower;
using namespace tui;

class SessionView { };

class App : public CommandHandler, public std::enable_shared_from_this<App> {
    TUI& _tui;
    std::shared_ptr<MessageView> _message;
    std::shared_ptr<HistoryView> _history;
    std::stack<std::shared_ptr<ContentProvider>> _view_stack {};

    std::shared_ptr<Process> _process;

    std::vector<SessionView> _sessions {};

    po::options_description _options_attach { "Allowed options" };
    po::positional_options_description _posiginal_attach {};

public:
    App(TUI& tui, pid_t pid)
        : _tui(tui)
        , _message(std::make_shared<MessageView>())
        , _history(std::make_shared<HistoryView>())
    {
        _options_attach.add_options()("help", "show help message")("pid,p", po::value<pid_t>(), "target process pid");
        _posiginal_attach.add("pid", -1);

        if (pid != -1) {
            _process = std::make_shared<Process>(pid);
        }

        push(_message);
    }

    StyleString tui_prompt(size_t width) override
    {
        using namespace ::tui::style;
        StyleStringBuilder builder {};
        builder << SetColor(ColorPrompt) << "Command" << ResetStyle() << "> ";
        return builder.str();
    }

    template <typename T>
    void push(T&& view)
    {
        _view_stack.push(std::dynamic_pointer_cast<ContentProvider>(view));
        _tui.show(_view_stack.top());
    }

    void pop(bool all = false)
    {
        while (_view_stack.size() > 1) {
            _view_stack.pop();
            if (not all) {
                break;
            }
        }

        _tui.show(_view_stack.top());
    }

    bool tui_key(int key) override {
        switch(key) {
            case KEY_UP:
            case KEY_DOWN:
                _history->history_key(key, _tui.editor());
                break;
        }
        return false;
    }

    void tui_run(const std::string& line) override
    {
        using namespace tui::style;

        auto [command, arguments] = dsl::parse_command(line);
        if (command.empty()) {
            return;
        }

        if (command == "exit") {
            _tui.exit();
        } else if (command == "msg" or command == "mesg" or command == "message") {
            push(_message);
        } else if (command == "history") {
            push(_history);
        } else if (command == "back") {
            pop();

        } else if (command == "attach") {
            po::variables_map vm {};
            PARSE_ARG(attach);

            pid_t pid = -1;

            try {
                if (vm.count("pid")) {
                    pid = vm["pid"].as<pid_t>();
                }
            } catch (const std::exception& e) {
                _message->stream()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                    << e.what();
                return;
            }

            if (pid == -1) {
                _message->stream() << "Usage: attach [options] process\n"
                                   << _options_attach;
                return;
            }

            pop(true);
            _sessions.clear();
            _process = std::make_shared<Process>(pid);

        } else if (command == "scan") {

        } else {
            using namespace tui::style;
            _message->stream()
                << EnableStyle(AttrUnderline) << SetColor(ColorWarning) << "Unknown command:"
                << ResetStyle() << " " << line;
            push(_message);
        }

        _history->push_back(command);
    }
};

int main(int argc, char* argv[])
{
    pid_t target_pid = -1;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()("help", "show help message")("pid,p", po::value<pid_t>(), "target process pid");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("pid")) {
            target_pid = vm["pid"].as<pid_t>();
        }
    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    // pid_t PID = 2197;
    // auto process = std::make_shared<Process>(PID);
    // Session session{process};
    // session.scan(ScanNumber<ComparatorEqual<uint32_t>, 4>{109u});
    // session.filter<FilterEqual>();

    TUI tui { TUIFlagColor };
    tui.attach(std::make_shared<App>(tui, target_pid));
    tui.run();
    return 0;
}
