/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stack>

#include "tui.hpp"

using namespace tui;
using namespace std::string_literals;

class MainMenu : public VisibleContainer<std::string> {
public:
    MainMenu()
    {
        this->emplace_back("list");
        this->emplace_back("history");
        this->emplace_back("exit");
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Main menu", width, 1, '=', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return AttributedString {};
        }
        return AttributedString { this->at(index) };
    }
};

class LongList : public VisibleContainer<std::string> {
public:
    LongList()
    {
        for (auto i = 0; i < 100; ++i) {
            if (i % 2) {
                this->emplace_back("Item "s + std::to_string(i));
            } else {
                this->emplace_back("Item "s + std::to_string(i) + "\n    Item "s + std::to_string(i) + " Second Line"s);
            }
        }
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Long List", width, 1, '=', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return AttributedString {};
        }
        return AttributedString { this->at(index) };
    }
};

class App : public CommandHandler, public std::enable_shared_from_this<App> {
    TUI& _tui;
    std::shared_ptr<MessageView> _message_view;
    std::shared_ptr<HistoryView> _history_view;
    std::shared_ptr<MainMenu> _main_menu;
    std::shared_ptr<LongList> _long_list;

    std::stack<std::shared_ptr<ContentProvider>> _view_stack {};

public:
    App(TUI& tui)
        : _tui(tui)
        , _message_view(std::make_shared<MessageView>())
        , _history_view(std::make_shared<HistoryView>())
        , _main_menu(std::make_shared<MainMenu>())
        , _long_list(std::make_shared<LongList>())
    {
        _history_view->set_flags(ContentProviderFlagAutoScrollTail);
        push(_message_view);
    }

    AttributedString tui_prompt(size_t width) override
    {
        using namespace ::tui::attributes;
        AttributedStringBuilder builder {};
        builder << SetColor(ColorPrompt) << "Command" << ResetStyle() << ": ";
        return builder.str();
    }

    template <typename T>
    void push(T&& view)
    {
        _view_stack.push(std::dynamic_pointer_cast<ContentProvider>(view));
        _tui.show(_view_stack.top());
    }

    void pop()
    {
        if (_view_stack.size() > 1) {
            _view_stack.pop();
        }
        _tui.show(_view_stack.top());
    }

    void tui_run(const std::string& command) override
    {
        _history_view->push_back(command);
        if (command == "exit") {
            _tui.exit();
        } else if (command == "list") {
            push(_long_list);
        } else if (command == "history") {
            push(_history_view);
        } else if (command == "help") {
            push(_main_menu);
        } else if (command == "back") {
            pop();
        } else {
            using namespace tui::attributes;
            _message_view->stream() << EnableStyle(AttrUnderline) << "Unknown command: " << ResetStyle() << command;
        }
    }
};

int main(int argc, char* argv[])
{
    TUI tui { TUIFlagColor };
    tui.attach(std::make_shared<App>(tui));
    tui.run();
    return 0;
}
