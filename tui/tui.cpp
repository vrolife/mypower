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
#include "tui.hpp"

namespace tui {

AttributedString AttributedString::layout(AttributedString& string, size_t width, size_t gap, int decoration, LayoutAlign align)
{
    auto new_string = string;
    if (new_string._string.size() >= width) {
        return new_string;
    }
    if (new_string._bytecode.empty()) {
        auto size = new_string._string.size();
        if (size <= UINT8_MAX) {
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString8));
            new_string._bytecode.push_back(static_cast<uint8_t>(size));
        } else if (size <= UINT16_MAX) {
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString16));
            auto buf = static_cast<uint16_t>(size);
            new_string._bytecode.append(reinterpret_cast<char*>(&buf), 2);
        } else if (size <= UINT32_MAX) {
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString32));
            auto buf = static_cast<uint32_t>(size);
            new_string._bytecode.append(reinterpret_cast<char*>(&buf), 4);
        }
#ifdef __LP64__
        else if (size <= UINT64_MAX) {
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString64));
            auto buf = static_cast<uint64_t>(size);
            new_string._bytecode.append(reinterpret_cast<char*>(&buf), 8);
        }
#endif
    }
    switch (align) {
    case LayoutAlign::Start: {
        auto space = width - new_string._string.size();
        if (space > 0) {
            auto repeat = static_cast<uint16_t>(std::min(space, gap));
            space -= repeat;
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            new_string._bytecode.push_back(' ');
            new_string._bytecode.append(reinterpret_cast<char*>(&repeat), 2);
        }
        if (space > 0) {
            auto repeat = static_cast<uint16_t>(space);
            new_string._bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            new_string._bytecode.push_back(decoration);
            new_string._bytecode.append(reinterpret_cast<char*>(&repeat), 2);
        }
        break;
    }
    case LayoutAlign::Center: {
        auto space = width - new_string._string.size();
        std::string bc;
        bc.reserve(new_string._bytecode.size() + 16);
        if (space <= (gap * 2)) { // no decoration
            auto repeat = static_cast<uint16_t>(gap / 2);

            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(' ');
            bc.append(reinterpret_cast<char*>(&repeat), 2);

            bc.append(new_string._bytecode.begin(), new_string._bytecode.end());

            repeat = space - repeat;

            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(' ');
            bc.append(reinterpret_cast<char*>(&repeat), 2);
        } else {
            auto repeat = static_cast<uint16_t>(space / 2 - gap);
            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(decoration);
            bc.append(reinterpret_cast<char*>(&repeat), 2);
            space -= repeat;

            repeat = gap;
            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(' ');
            bc.append(reinterpret_cast<char*>(&repeat), 2);
            space -= repeat;

            bc.append(new_string._bytecode.begin(), new_string._bytecode.end());

            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(' ');
            bc.append(reinterpret_cast<char*>(&repeat), 2);
            space -= repeat;

            repeat = static_cast<uint16_t>(space);

            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(decoration);
            bc.append(reinterpret_cast<char*>(&repeat), 2);
        }
        new_string._bytecode = bc;
        break;
    }
    case LayoutAlign::End: {
        auto space = width - new_string._string.size();
        std::string bc;
        bc.reserve(new_string._bytecode.size() + 8);
        if (space > 0) {
            auto repeat = static_cast<uint16_t>(space - gap);
            space -= repeat;
            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(decoration);
            bc.append(reinterpret_cast<char*>(&repeat), 2);
        }
        if (space > 0) {
            auto repeat = static_cast<uint16_t>(space);
            bc.push_back(static_cast<uint8_t>(AttributedString::BytecodeRepeat16));
            bc.push_back(' ');
            bc.append(reinterpret_cast<char*>(&repeat), 2);
        }
        bc.append(new_string._bytecode.begin(), new_string._bytecode.end());
        new_string._bytecode = bc;
        break;
    }
    }
    return new_string;
}

void AttributedStringBuilder::print_attributed_string(WINDOW* win, const AttributedString& string, bool reverse)
{
    if (string._bytecode.empty()) {
        if (reverse) {
            wattron(win, A_REVERSE);
        }
        waddstr(win, string._string.c_str());
        if (reverse) {
            wattroff(win, A_REVERSE);
        }
    } else {
        auto* str = string._string.data();
        auto* ends = str + string._string.size();
        auto* code = string._bytecode.data();
        auto* endc = code + string._bytecode.size();
        NCURSES_CH_T default_attrs = reverse ? A_REVERSE : 0;

        wattrset(win, default_attrs);

        while (code < endc) {
            switch (static_cast<AttributedString::Bytecode>(code[0])) {
            case AttributedString::Bytecode::BytecodeNop:
                str += 1;
                code += 1;
                break;
            case AttributedString::Bytecode::BytecodeString8: {
                auto size = code[1];
                auto space = size;
                waddnstr(win, str, space);
                str += size;
                code += 2;
                break;
            }
            case AttributedString::Bytecode::BytecodeString16: {
                uint16_t size = 0;
                memcpy(&size, &code[1], 2);
                auto space = size;
                waddnstr(win, str, space);
                str += size;
                code += 3;
                break;
            }
            case AttributedString::Bytecode::BytecodeString32: {
                uint32_t size = 0;
                memcpy(&size, &code[1], 4);
                auto space = size;
                waddnstr(win, str, space);
                str += size;
                code += 5;
                break;
            }
#ifdef __LP64__
            case AttributedString::Bytecode::BytecodeString64: {
                uint64_t size = 0;
                memcpy(&size, &code[1], 8);
                auto space = size;
                waddnstr(win, str, space);
                str += size;
                code += 7;
                break;
            }
#endif
            case AttributedString::BytecodeAttrOn: {
                NCURSES_CH_T attrs = 0;
                memcpy(&attrs, &code[1], sizeof(attrs));
                if (reverse && (attrs & A_REVERSE)) {
                    wattroff(win, A_REVERSE);
                    attrs &= ~A_REVERSE;
                }
                wattron(win, attrs);
                code += (1 + sizeof(attrs));
                break;
            }
            case AttributedString::BytecodeAttrOff: {
                NCURSES_CH_T attrs = 0;
                memcpy(&attrs, &code[1], sizeof(attrs));
                if (reverse && (attrs & A_REVERSE)) {
                    wattron(win, A_REVERSE);
                    attrs &= ~A_REVERSE;
                }
                wattroff(win, attrs);
                code += (1 + sizeof(attrs));
                break;
            }
            case AttributedString::BytecodeAttrSet: {
                NCURSES_CH_T attrs = 0;
                memcpy(&attrs, &code[1], sizeof(attrs));
                if (reverse) {
                    attrs = (attrs & ~A_REVERSE) | (~attrs & A_REVERSE);
                }
                wattrset(win, attrs);
                code += (1 + sizeof(attrs));
                break;
            }
            case AttributedString::BytecodeAttrClear: {
                wattrset(win, default_attrs);
                code += 1;
                break;
            }
            case AttributedString::BytecodeRepeat16: {
                auto ch = code[1];
                uint16_t count = 0;
                memcpy(&count, &code[2], sizeof(count));
                while (count) {
                    waddch(win, ch);
                    count -= 1;
                }
                code += 4;
                break;
            }
            }
        }

        wattrset(win, 0);
    }
}

void ContentProvider::tui_notify_changed()
{
    if (_tui) {
        _tui->invalidate();
    }
}

TUI::TUI(int flags)
{
    initscr();
    use_default_colors();
    // cbreak();
    raw();
    noecho();
    set_escdelay(100);

    if (has_colors() and flags & TUIFlagColor) {
        start_color();
        init_pair(attributes::ColorNormal, COLOR_WHITE, -1);
        init_pair(attributes::ColorHighlight, COLOR_BLACK, COLOR_WHITE);
        init_pair(attributes::ColorWarning, COLOR_YELLOW, -1);
        init_pair(attributes::ColorError, COLOR_RED, -1);
        init_pair(attributes::ColorInfo, COLOR_BLUE, -1);
        init_pair(attributes::ColorPrompt, COLOR_GREEN, -1);
        init_pair(attributes::ColorImportant, COLOR_WHITE, COLOR_RED);
    }

    _win_title = newwin(1, 1, 0, 0);
    _win_stage = newwin(1, 1, 0, 0);
    _win_editor = newwin(1, 1, 0, 0);

    _win_canvas = newpad(1, 1);

    if (_win_title == nullptr or _win_stage == nullptr or _win_editor == nullptr or _win_canvas == nullptr) {
        throw std::runtime_error("out of memory");
    }

    keypad(_win_editor, TRUE);

    resize();
}

TUI::~TUI()
{
    _handler.reset();
    if (_win_title != nullptr) {
        delwin(_win_title);
        _win_title = nullptr;
    }
    if (_win_stage != nullptr) {
        delwin(_win_stage);
        _win_stage = nullptr;
    }
    if (_win_editor != nullptr) {
        delwin(_win_editor);
        _win_editor = nullptr;
    }
    if (_win_canvas != nullptr) {
        delwin(_win_canvas);
        _win_canvas = nullptr;
    }
    endwin();
}

void TUI::update_title()
{
    if (_provider) {
        int width, height;
        getmaxyx(_win_canvas, height, width);
        _title_string = _provider->tui_title(width);
    }
}

void TUI::mode_switched() {
    if (not _command_mode) {
        _content_selected_index = _content_cached_index;
        // find first visible item
        size_t lines_above = 0;
        size_t hidden_items = 0;
        for (size_t idx = 0; idx < _content_cached_items.size() and lines_above < _content_scroll_lines; ++idx) {
            hidden_items += 1;
            lines_above += _content_cached_items.at(idx)._height;
        }
        _content_selected_index += hidden_items;
        if (_content_selected_index >= (_provider->tui_count() - 1)) {
            _content_selected_index = _provider->tui_count() - 1;
        }
    }
    invalidate();
}

int TUI::run()
{
    int key;

    if (_handler) {
        _handler->tui_start();
    }

    _exit = false;

    while (not _exit) {
        wtimeout(_win_editor, _provider ? _provider->tui_timeout() : -1);
        draw();
        key = wgetch(_win_editor);

        switch(key) {
            case ERR:
                continue;
            case 27: // ESC
                _command_mode = !_command_mode;
                mode_switched();
                continue;
            case TUI_KEY_CTRL('d'):
                if (_editor.buffer().empty()) {
                    return 0;
                }
                continue;
            case TUI_KEY_CTRL('c'): {
                if (not _command_mode) {
                    _command_mode = true;
                    mode_switched();
                }
                _editor.reset();
                continue;
            }
        }

        if (_command_mode) {
            if (_handler == nullptr or not _handler->tui_key(key)) {
                if (key == KEY_ENTER or key == '\n') {
                    if (_handler) {
                        _handler->tui_run(_editor.buffer());
                        _editor.reset();
                    }
                } else {
                    _editor.input(key);
                }
            }

        } else { // List mode
            if (_provider == nullptr or not _provider->tui_key(_content_selected_index, key)) {
                list_mode_key(key);
            }
        }

        switch (key) {
        case KEY_RESIZE:
            resize();
            break;
        }
    }
    return 0;
}

void TUI::list_mode_key(int key)
{
    int screen_height, screen_width, offset = 0;
    getmaxyx(stdscr, screen_height, screen_width);
    const auto content_height = screen_height - _title_height - _editor.height();

    switch (key) {
    case KEY_UP: {
        if (_content_selected_index == 0) {
            return;
        }
        _content_selected_index -= 1;
        if (_content_selected_index < _content_cached_index) {
            _content_cached_index = _content_selected_index;
            _content_scroll_lines = 0;
            invalidate();
        } else {
            invalidate();
            size_t lines_above = 0;
            for (size_t idx = 0; idx < _content_cached_items.size() and (idx + _content_cached_index) < _content_selected_index; ++idx) {
                lines_above += _content_cached_items.at(idx)._height;
            }
            if (lines_above < _content_scroll_lines) {
                _content_scroll_lines = lines_above;
            }
        }
        break;
    }
    case KEY_DOWN: {
        auto count = _provider->tui_count();
        if (_content_selected_index >= (count - 1)) {
            return;
        }
        _content_selected_index += 1;
        if (_content_selected_index >= (_content_cached_index + _content_cached_items.size())) {
            _content_cached_index += 1;
            invalidate();
            _content_scroll_lines = _content_cached_lines - content_height;
        } else {
            invalidate();
            size_t lines_above = 0;
            for (size_t idx = 0; idx < _content_cached_items.size() and (idx + _content_cached_index) <= _content_selected_index; ++idx) {
                lines_above += _content_cached_items.at(idx)._height;
            }
            if (lines_above > content_height) {
                _content_scroll_lines = lines_above - content_height;
            }
        }
        break;
    }
    case KEY_ENTER:
    case '\n': {
        auto command = _provider->tui_select(_content_selected_index);
        if (not command.empty()) {
            if (_handler && _provider) {
                _handler->tui_run(command);
            }
            _command_mode = true;
            invalidate();
        }
        break;
    }
    }
}
void TUI::resize()
{
    int screen_height, screen_width, offset = 0;
    getmaxyx(stdscr, screen_height, screen_width);

    mvwin(_win_title, offset, 0);
    wresize(_win_title, _title_height, screen_width);
    offset += _title_height;

    // content area
    const auto content_height = screen_height - _title_height - _editor.height();
    mvwin(_win_stage, offset, 0);
    wresize(_win_stage, content_height, screen_width);
    offset += content_height;

    mvwin(_win_editor, offset, 0);
    wresize(_win_editor, _editor.height(), screen_width);

    auto provider = _provider;
    show(provider);
}

void TUI::draw()
{
    if (_provider) {
        werase(_win_title);
        wmove(_win_title, 0, 0);
        AttributedStringBuilder::print_attributed_string(_win_title, _title_string, !_command_mode);
        wnoutrefresh(_win_title);

        size_t stage_width, stage_height;
        getmaxyx(_win_stage, stage_height, stage_width);

        werase(_win_stage);
        wnoutrefresh(_win_stage);
        pnoutrefresh(_win_canvas, _content_scroll_lines, 0, _title_height, 0, _title_height + stage_height, stage_width);
    }

    if (_handler) {
        size_t width, height;
        getmaxyx(_win_editor, height, width);

        werase(_win_editor);

        if (_command_mode) {
            auto prompt = _handler->tui_prompt(width);

            AttributedStringBuilder::print_attributed_string(_win_editor, prompt);

            wprintw(_win_editor, "%s ", _editor.buffer().c_str());
            wmove(_win_editor, 0, _editor.cursor() + prompt.size());
        } else {
            waddstr(_win_editor, "Press ESC to enter command mode");
        }
        wnoutrefresh(_win_editor);
    }

    doupdate();
}

void TUI::invalidate()
{
    size_t screen_width, screen_height;
    getmaxyx(stdscr, screen_height, screen_width);

    const auto content_height = screen_height - _title_height - _editor.height();

    auto count = _provider->tui_count();

    _content_cached_items.clear();
    _content_cached_items.reserve(content_height);
    _content_cached_lines = 0;

    auto show_tail = (_provider->_flags & ContentProviderFlagAutoScrollTail) && _command_mode;

    if (count <= content_height) {
        _content_cached_index = 0;
    } else {
        if (show_tail) {
            _content_cached_index = count - content_height;
            _content_scroll_lines = 0;
        }
    }

    size_t cache_max = std::min(content_height, count);

    if ((_content_cached_index + cache_max) >= count) {
        _content_cached_index = count - cache_max;
    }

    for (auto idx = 0; idx < cache_max; ++idx) {
        detail::ItemCache item {};
        item._conetnt = _provider->tui_item(idx + _content_cached_index, screen_width);
        item._height = item._conetnt.calc_height_slow();
        _content_cached_lines += item._height;
        _content_cached_items.emplace_back(std::move(item));
    }

    if (show_tail && _content_cached_lines > content_height) {
        _content_scroll_lines = _content_cached_lines - content_height;
    }

    // render
    wresize(_win_canvas, _content_cached_lines, screen_width);
    werase(_win_canvas);

    int y = 0;
    int idx = 0;
    for (auto& cache : _content_cached_items) {
        wmove(_win_canvas, y, 0);
        AttributedStringBuilder::print_attributed_string(_win_canvas, cache._conetnt,
            ((idx + _content_cached_index) == _content_selected_index) and not _command_mode);
        idx += 1;
        y += cache._height;
    }
}

HistoryView::HistoryView()
{
    set_flags(ContentProviderFlagAutoScrollTail);
}

AttributedString HistoryView::tui_title(size_t width)
{
    return AttributedString::layout("History", width, 1, '=', LayoutAlign::Center);
}

void HistoryView::history_key(int key, Editor& editor)
{
    switch (key) {
    case KEY_UP:
        if (empty()) {
            break;
        }
        if (_index == -1) {
            _saved_buffer = editor.buffer();
            _index = size() - 1;
        } else if (_index > 0) {
            _index -= 1;
        }
        editor.update(at(_index), SIZE_MAX);
        break;
    case KEY_DOWN:
        if (_index != -1) {
            if (_index < (size() - 1)) {
                _index += 1;
                editor.update(at(_index), SIZE_MAX);
            } else {
                _index = -1;
                editor.update(_saved_buffer, SIZE_MAX);
            }
        }
        break;
    }
}

void HistoryView::tui_notify_changed()
{
    VisibleContainer<std::string>::tui_notify_changed();
    _index = -1;
}

} // namespace tui
