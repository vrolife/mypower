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

namespace mypower {

class CommandSession : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandSession(Application& app)
        : Command(app, "session")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("session,s", po::value<std::string>(), "session index/name");
        _options.add_options()("name,n", po::value<std::string>(), "set session name");
        _options.add_options()("list,l", po::bool_switch()->default_value(false), "List exists sessions");
        _options.add_options()("delete,d", po::bool_switch()->default_value(false), "delete session");
        _posiginal.add("session", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "session";
    }

    void show_short_help() override {
        message() << "session\t\t\tList/Select/Delete session";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        if (opts.count("help")) {
            message() << "Usage: " << command << " [options] name/idx\n"
                      << _options;
            show();
            return;
        }

        if (_app._session_views.empty()) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " No avaliable session, create a session using the 'scan' command";
            show(_app._message_view);
            return;
        }

        if (opts["list"].as<bool>() or not opts.count("session")) {
            message() << "Sessions:";
            auto index = 0;
            for (auto& session : _app._session_views) {
                message()
                    << SetColor(ColorInfo)
                    << "< " << index++ << " >"
                    << ResetStyle() << " "
                    << SetStyle(AttrUnderline)
                    << session->session_name();
            }
            show(_app._message_view);
            return;
        }

        ssize_t index = -1;
        auto name_or_index = opts["session"].as<std::string>();
        try {
            index = std::stoul(name_or_index);
        } catch (...) {
            index = 0;
            for (auto& session : _app._session_views) {
                if (session->session_name() == name_or_index) {
                    break;
                }
                index += 1;
            }
        }

        if (index < 0 or index >= _app._session_views.size()) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Invalid session name/index";
            show(_app._message_view);
            return;
        }

        if (opts["delete"].as<bool>()) {
            if (not _app._session_views.empty()) {
                auto iter = _app._session_views.begin() + index;
                _app._session_views.erase(iter);

                if (_app._session_views.empty()) {
                    _app._current_session_view.reset();
                } else {
                    _app._current_session_view = _app._session_views.at(0);
                }
                show(_app._current_session_view);
            }

        } else {
            _app._current_session_view = _app._session_views.at(index);

            if (opts.count("name")) {
                _app._current_session_view->session_name(opts["name"].as<std::string>());
                _app._tui.update_title();
            }

            show(_app._current_session_view);
        }
    }
};

static RegisterCommand<CommandSession> _Session {};

} // namespace mypower
