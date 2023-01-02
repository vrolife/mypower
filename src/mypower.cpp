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
#include <getopt.h>

#include <iostream>
#include <regex>
#include <stack>

#include "scanner.hpp"
#include "tui.hpp"
#include "dsl.hpp"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace mypower;
using namespace tui;

class SessionView { };

class App : public CommandHandler, public std::enable_shared_from_this<App> {
    TUI& _tui;
    std::shared_ptr<MessageView> _message;
    std::shared_ptr<HistoryView> _history;
    std::stack<std::shared_ptr<ContentProvider>> _view_stack{};

    std::shared_ptr<Process> _process;

    std::vector<SessionView> _sessions{};

public:
    App(TUI& tui, pid_t pid)
        : _tui(tui)
        , _message(std::make_shared<MessageView>())
        , _history(std::make_shared<HistoryView>())
    {
        if (pid != -1) {
            _process = std::make_shared<Process>(pid);
        }

        push(_message);
    }

    StyleString tui_prompt(size_t width) override {
        using namespace ::tui::style;
        StyleStringBuilder builder{};
        builder << SetColor(ColorPrompt) << "Command" << ResetStyle() << ": ";
        return builder.str();
    }

    template<typename T>
    void push(T&& view) {
        _view_stack.push(std::dynamic_pointer_cast<ContentProvider>(view));
        _tui.show(_view_stack.top());
    }

    void pop(bool all=false) {
        while(_view_stack.size() > 1) {
            _view_stack.pop();
            if (not all) {
                break;
            }
        }
        
        _tui.show(_view_stack.top());
    }

    void tui_run(const std::string& line) override
    {
        using namespace tui::style;

        auto arguments = dsl::parse_command(line);
        if (arguments.empty()) {
            return;
        }

        auto& command = arguments.at(0);

        if (command == "exit") {
            _tui.exit();
        } else if (command == "msg" or command == "mesg" or command == "message") {
            push(_message);
        } else if (command == "history") {
            push(_history);
        } else if (command == "back") {
            pop();

        } else if (command == "attach") {
            pid_t pid = -1;
            if (arguments.size() != 2) {
                _message->stream() 
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() 
                    << " Require " 
                    << EnableStyle(AttrUnderline) << SetColor(ColorWarning) << "one" << ResetStyle() 
                    << " argument";
                _message->stream() << "Usage: attach pid";
                return;
            }
            try {
                pid = std::stoi(arguments[1]);
            } catch(const std::out_of_range&)
            {
                _message->stream() 
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() << " "
                    << EnableStyle(AttrUnderline) << SetColor(ColorWarning) << "pid" << ResetStyle() 
                    << " out of range";
                return;

            } catch(const std::invalid_argument&) {
                _message->stream() 
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()  
                    << "Invalid argument: " << arguments[1];
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
                << ResetStyle() << " " << command;
            push(_message);
        }

        _history->push_back(command);
    }
};

void print_usage() {
    std::cout << "Usage: mypower -p pid" << std::endl;
    std::cout << "Usage: mypower --pid pid" << std::endl;
    std::cout << "Usage: mypower --help" << std::endl;
}

int main(int argc, char* argv[])
{
    try {

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("compression", po::value<double>(), "set compression level")
        ;

        po::variables_map vm;        
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);    

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (vm.count("compression")) {
            std::cout << "Compression level was set to " 
                 << vm["compression"].as<double>() << ".\n";
        } else {
            std::cout << "Compression level was not set.\n";
        }
    }
    catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }
    return 0;
    // pid_t PID = 2197;
    // auto process = std::make_shared<Process>(PID);
    // Session session{process};
    // session.scan(ScanNumber<ComparatorEqual<uint32_t>, 4>{109u});
    // session.filter<FilterEqual>();

    pid_t target_pid = -1;

    static struct option long_options[] = {
        { "pid", required_argument, 0, 0 },
        { "help", required_argument, 0, 0 },
        { 0, 0, 0, 0 },
    };

    while(1) {
        int option_index = 0;
        auto ret = getopt_long(argc, argv, "hp:", long_options, &option_index);
        if (ret == -1) {
            break;
        }

        switch(ret) {
            case 0:
            case 'p':
                target_pid = std::stoi(optarg);
                break;
            case 1:
            case 'h':
                print_usage();
                exit(0);
                break;
        }
    }

    TUI tui {TUIFlagColor};
    tui.attach(std::make_shared<App>(tui, target_pid));
    tui.run();
    return 0;
}
