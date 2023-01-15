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
#ifndef __mypower_hpp__
#define __mypower_hpp__

#include <functional>
#include <tuple>

#include "process.hpp"
#include "tui.hpp"

#define PROGRAM_OPTIONS()                                                                       \
    using namespace tui::style; \
    po::variables_map opts {}; \
    try {                                                                                     \
        po::store(po::command_line_parser(arguments)                                          \
                      .options(_options)                                               \
                      .positional(_posiginal)                                          \
                      .run(),                                                                 \
            opts);                                                                              \
        notify(opts);                                                                           \
    } catch (const std::exception& exc) {                                                     \
        message()                                                               \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " " << exc.what();                                                             \
        show();                                                                  \
        return;                                                                               \
    } catch (...) {                                                                           \
        message()                                                               \
            << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() \
            << " Unknown error";                                                              \
        show();                                                                  \
        return;                                                                               \
    }

namespace mypower {
using namespace tui;

class Application;

struct SessionView : public ContentProvider {
    virtual const std::string session_name() = 0;
    virtual void session_name(const std::string& name) = 0;
    virtual void session_reset() = 0;
};

struct Command {
    Application& _app;
    Command(Application& app) : _app(app) { }

    virtual ~Command() = default;
    virtual void run(const std::string& command, const std::vector<std::string>& args) = 0;

    virtual bool match(const std::string& command) = 0;

    MessageStream message();

    template <typename T>
    void show(T&& view);

    void show();
};

typedef std::function<std::unique_ptr<Command>(Application&)> CommandInitializer;

struct Application {
    TUI& _tui;

    std::shared_ptr<Process> _process;

    std::shared_ptr<MessageView> _message_view;
    std::shared_ptr<ContentProvider> _current_view {};

    std::vector<std::shared_ptr<SessionView>> _session_views {};
    std::shared_ptr<SessionView> _current_session_view {};

    Application(TUI& tui) : _tui(tui) { }

    void show(std::shared_ptr<ContentProvider>&& view) {
        show(view);
    }

    void show(std::shared_ptr<ContentProvider>& view) {
        if (view == nullptr) {
            _tui.show(_message_view);
            return;
        }
        _current_view = view;
        _tui.show(_current_view);
    }

private:
    template<typename T>
    friend class RegisterCommand;

    static
    void register_command(CommandInitializer);
};

inline
MessageStream Command::message() {
    return _app._message_view->stream();
}

template <typename T>
inline
void Command::show(T&& view)
{
    _app.show(std::dynamic_pointer_cast<ContentProvider>(view));
}
inline
void Command::show()
{
    _app.show(_app._message_view);
}

template<typename T>
struct RegisterCommand {
    RegisterCommand() {
        Application::register_command(
            &RegisterCommand<T>::register_command
        );
    }

    static std::unique_ptr<Command> register_command(Application& app) {
        return std::make_unique<T>(app);
    }
};

} // namespace mypower

#endif
