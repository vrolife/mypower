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
#include <random>
#include <regex>
#include <stack>

#include <boost/program_options.hpp>

#include "dsl.hpp"
#include "mypower.hpp"
#include "tui.hpp"

namespace po = boost::program_options;

using namespace std::string_literals;
using namespace tui;

namespace mypower {

std::vector<CommandInitializer> _command_list {};

void Application::register_command(CommandInitializer init)
{
    _command_list.emplace_back(std::move(init));
}

class App : public CommandHandler, public std::enable_shared_from_this<App>, public Application {

    std::vector<std::unique_ptr<Command>> _commands {};

    std::shared_ptr<HistoryView> _history_view;

    po::options_description _options_update { "Allowed options" };
    po::positional_options_description _posiginal_update {};

    po::options_description _options_dump { "Allowed options" };
    po::positional_options_description _posiginal_dump {};

public:
    App(TUI& tui, pid_t pid)
        : Application(tui)
    {
        for (auto& init : _command_list) {
            _commands.emplace_back(init(*this));
        }

        _message_view = std::make_shared<MessageView>();
        _history_view = std::make_shared<HistoryView>();

        if (pid != -1) {
            _process = std::make_shared<Process>(pid);
        }

        show(_message_view);
    }

    ~App() {
        for (auto& cmd : _commands) {
            cmd.reset();
        }
    }

    AttributedString tui_prompt(size_t width) override
    {
        using namespace ::tui::attributes;
        AttributedStringBuilder builder {};
        builder
            << SetColor(ColorPrompt)
            << (_current_session_view ? _current_session_view->session_name() : "") << ResetStyle()
            << "> ";
        return builder.str();
    }

    template <typename T>
    void show(T&& view)
    {
        if (view == nullptr) {
            _tui.show(_message_view);
            return;
        }
        _current_view = std::dynamic_pointer_cast<ContentProvider>(view);
        _tui.show(_current_view);
    }

    void complete()
    {
        auto& editor = _tui.editor();
        const auto& buffer = editor.buffer();

        static std::vector<std::string> keyworks = {
            "help",
            "exit",
            "msg", "mesg", "message",
            "history",
            "back",
            "attach",
            "selfattach",
            "ps", "findps", "findpsex",
            "test",
            "scan",
            "filter",
            "update"
        };

        if (buffer.empty()) {
            return;
        }

        for (auto& str : keyworks) {
            if (str.find(buffer) == 0) {
                _tui.editor().update(str, -1);
                break;
            }
        }
    }

    bool tui_key(int key) override
    {
        switch (key) {
        case KEY_UP:
        case KEY_DOWN:
            _history_view->history_key(key, _tui.editor());
            break;
        case '\t':
            complete();
            return true;
        }
        return false;
    }

    void show_help() {
        for (auto& cmd : _commands) {
            cmd->show_short_help();
        }
    }

    void tui_run(const std::string& line) override
    {
        using namespace tui::attributes;

        if (line.empty()) {
            if (_current_view == _message_view) {
                show(_current_session_view);
            } else {
                show(_message_view);
            }
            return;
        }

        _history_view->push_back(line);

        auto [command, arguments] = dsl::parse_command(line);

        if (command == "exit") {
            _tui.exit();

        } else if (command == "msg" or command == "mesg" or command == "message") {
            show(_message_view);

        } else if (command == "history") {
            show(_history_view);
        } else if (command == "help") {
            show_help();

        } else if (command == "refresh-view") {
            if (_current_view) {
                _current_view->tui_notify_changed();
            }

        } else if (command == "reset") {
            if (_current_session_view) {
                _current_session_view->session_reset();
                show(_current_session_view);

            } else {
                _message_view->stream()
                    << SetColor(ColorError)
                    << "Error:"
                    << ResetStyle()
                    << " No selected session. select a session using \"session\" command";
                show(_message_view);
            }

        } else {
            for (auto& cmd : _commands) {
                if (cmd->match(command)) {
                    cmd->run(command, arguments);
                    return;
                }
            }

            using namespace tui::attributes;
            _message_view->stream()
                << SetColor(ColorWarning) << "Unknown command:"
                << ResetStyle() << " " << line;
            show(_message_view);
        }
    }
};

} // namespace mypower

__attribute__((weak)) int main(int argc, char* argv[])
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

    TUI tui { TUIFlagColor };
    tui.attach(std::make_shared<mypower::App>(tui, target_pid));
    tui.run();
    return 0;
}
