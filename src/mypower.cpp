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
#include <random>

#include <boost/program_options.hpp>

#include "dsl.hpp"
#include "processlist.hpp"
#include "scanner.hpp"
#include "tui.hpp"

#define PARSE_ARG(name)                                                                       \
    try {                                                                                     \
        po::store(po::command_line_parser(arguments)                                          \
                      .options(_options_##name)                                               \
                      .positional(_posiginal_##name)                                          \
                      .run(),                                                                 \
            vm);                                                                              \
        notify(vm);                                                                           \
    } catch (const std::exception& exc) {                                                     \
        _message_view->stream()                                                                    \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " " << exc.what();                                                             \
        return;                                                                               \
    } catch (...) {                                                                           \
        _message_view->stream()                                                                    \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " Unknown error";                                                              \
        return;                                                                               \
    }

namespace po = boost::program_options;

using namespace mypower;
using namespace tui;

class TestView : public VisibleContainer<ssize_t> {
public:
    TestView()
    {
        std::random_device rd{};
        std::uniform_int_distribution<ssize_t> dist(0, 1000);
        emplace_back(dist(rd));
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("Test", width, 1, '-', LayoutAlign::Center);
    }

    StyleString tui_item(size_t index, size_t width) override
    {
        switch (index) {
        case 0:
            return StyleString { std::to_string(at(0)) + " (INT32)"s};
        case 1:
            return StyleString { "+1" };
        case 2:
            return StyleString { "-1" };
        case 3:
            return StyleString { "+5" };
        case 4:
            return StyleString { "-5" };
        }
        return {};
    }

    std::string tui_select(size_t index) override {
        switch(index) {
            case 1:
                at(0) += 1;
                tui_notify_changed();
                break;
            case 2:
                at(0) -= 1;
                tui_notify_changed();
                break;
            case 3:
                at(0) += 5;
                tui_notify_changed();
                break;
            case 4:
                at(0) -= 5;
                tui_notify_changed();
                break;
        }
        return {};
    }
};

class SessionView { };

class App : public CommandHandler, public std::enable_shared_from_this<App> {
    TUI& _tui;
    std::shared_ptr<MessageView> _message_view;
    std::shared_ptr<HistoryView> _history_view;
    std::shared_ptr<TestView> _test_view;
    std::stack<std::shared_ptr<ContentProvider>> _view_stack {};

    std::shared_ptr<Process> _process;

    std::vector<SessionView> _sessions {};

    po::options_description _options_attach { "Allowed options" };
    po::positional_options_description _posiginal_attach {};

    po::options_description _options_findps { "Allowed options" };
    po::positional_options_description _posiginal_findps {};

public:
    App(TUI& tui, pid_t pid)
        : _tui(tui)
        , _message_view(std::make_shared<MessageView>())
        , _history_view(std::make_shared<HistoryView>())
        , _test_view(std::make_shared<TestView>())
    {
        _options_attach.add_options()("help", "show help message")("pid,p", po::value<pid_t>(), "target process pid");
        _posiginal_attach.add("pid", -1);

        _options_findps.add_options()("help", "show help message")("filter,f", po::value<std::string>(), "regex filter");
        _posiginal_findps.add("filter", -1);

        if (pid != -1) {
            _process = std::make_shared<Process>(pid);
        }

        push(_message_view);
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
        if (not _view_stack.empty() and _view_stack.top() == view) {
            return;
        }
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

    bool tui_key(int key) override
    {
        switch (key) {
        case KEY_UP:
        case KEY_DOWN:
            _history_view->history_key(key, _tui.editor());
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

        if (command == "selfattach") {
            command = "attach";
            arguments = { std::to_string(getpid()) };
        }

        if (command == "exit") {
            _tui.exit();

        } else if (command == "msg" or command == "mesg" or command == "message") {
            push(_message_view);

        } else if (command == "history") {
            push(_history_view);

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
                _message_view->stream()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                    << e.what();
                return;
            }

            if (pid == -1) {
                _message_view->stream() << "Usage: attach [options] pid\n"
                                   << _options_attach;
                return;
            }

            pop(true);
            _sessions.clear();
            _process = std::make_shared<Process>(pid);

        } else if (command == "ps") {
            push(std::make_shared<ProcessList>());

        } else if (command == "findpsex" or command == "findps") {
            po::variables_map vm {};
            PARSE_ARG(findps);
            std::string filter {};

            try {
                if (vm.count("filter")) {
                    filter = vm["filter"].as<std::string>();
                }
            } catch (const std::exception& e) {
                _message_view->stream()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                    << e.what();
                return;
            }

            if (filter.empty()) {
                _message_view->stream() << "Usage: " << command << " [options] regex\n"
                                   << _options_findps;
                return;
            }

            if (command == "findps") {
                filter = ".*"s + filter + ".*"s;
            }

            push(std::make_shared<ProcessList>(filter));

        } else if (command == "test") {
            push(_test_view);

        } else {
            using namespace tui::style;
            _message_view->stream()
                << EnableStyle(AttrUnderline) << SetColor(ColorWarning) << "Unknown command:"
                << ResetStyle() << " " << line;
            push(_message_view);
        }

        _history_view->push_back(command);
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