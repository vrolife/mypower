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
#ifndef __tui_hpp__
#define __tui_hpp__

#include <cstring>
#include <cassert>

#include <algorithm>

#include <ncurses/ncurses.h>

#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <memory>

namespace tui {
class StyleStringBuilder;

namespace style {
    enum Attribute {
        AttrNormal = A_NORMAL,	
        AttrStandout = A_STANDOUT,	
        AttrUnderline = A_UNDERLINE,	
        AttrReverse = A_REVERSE,	
        AttrBlink = A_BLINK,		
        AttrDim = A_DIM,		
        AttrBold = A_BOLD,		
        AttrAltcharset = A_ALTCHARSET,
        AttrInvis = A_INVIS,		
        AttrProtect = A_PROTECT,	
        AttrHorizontal = A_HORIZONTAL,
        AttrLeft = A_LEFT,		
        AttrLow = A_LOW,		
        AttrRight = A_RIGHT,		
        AttrTop = A_TOP,		
        AttrVertical = A_VERTICAL,	
    };

    enum Color {
        ColorNormal = 1,
        ColorHighlight,
        ColorWarning,
        ColorError,
        ColorInfo,
        ColorPrompt,
        ColorImportant
    };

    class SetStyle {
        friend class ::tui::StyleStringBuilder;
        NCURSES_CH_T _style;
    public:
        SetStyle(NCURSES_CH_T style) : _style(style) { }
    };

    class SetColor {
        friend class ::tui::StyleStringBuilder;
        NCURSES_CH_T _style;
    public:
        SetColor(Color color) : _style(COLOR_PAIR(color)) { }
    };

    class EnableStyle {
        friend class ::tui::StyleStringBuilder;
        NCURSES_CH_T _style;
    public:
        EnableStyle(NCURSES_CH_T style) : _style(style) { }
    };

    class DisableStyle {
        friend class ::tui::StyleStringBuilder;
        NCURSES_CH_T _style;
    public:
        DisableStyle(NCURSES_CH_T style) : _style(style) { }
    };

    struct ResetStyle { };

} // namespace style

enum class LayoutAlign {
    Start,
    Center,
    End
};

class StyleString {
    friend class StyleStringBuilder;

    std::string _string{};
    std::string _bytecode{};
    
    enum Bytecode {
        BytecodeNop = 0,
        BytecodeString8 = 1,
        BytecodeString16 = 2,
        BytecodeString32 = 3,
        BytecodeString64 = 4,
        BytecodeAttrOn = 5,
        BytecodeAttrOff = 6,
        BytecodeAttrSet = 7,
        BytecodeAttrClear = 8,
        BytecodeRepeat16 = 9,
    };

    StyleString(std::string&& string, std::string&& bytecode)
    : _string(std::move(string)), _bytecode(std::move(bytecode))
    { }

    StyleString(std::string&& string, std::string& bytecode)
    : _string(std::move(string)), _bytecode(bytecode)
    { }

public:
    StyleString() = default;

    StyleString(const std::string& string)
    : _string(string)
    { }

    StyleString(const StyleString&) = default;
    StyleString(StyleString&&) noexcept = default;
    StyleString& operator=(const StyleString&) = default;
    StyleString& operator=(StyleString&&) noexcept = default;

    operator std::string&() {
        return _string;
    }
    
    operator const std::string&() const {
        return _string;
    }

    size_t calc_height_slow() const {
        return std::count(_string.begin(), _string.end(), '\n') + 1;
    }

    size_t size() const { return _string.size(); }

    size_t empty() const { return _string.empty(); }

    bool style() const {
        return not _bytecode.empty();
    }

    static
    StyleString layout(StyleString&& string, size_t width, size_t gap, int decoration, LayoutAlign align) {
        return layout(string, width, gap, decoration, align);
    }

    static
    StyleString layout(std::string&& string, size_t width, size_t gap, int decoration, LayoutAlign align) {
        return layout(string, width, gap, decoration, align);
    }

    static
    StyleString layout(StyleString& string, size_t width, size_t gap, int decoration, LayoutAlign align) {
        auto new_string = string;
        if (new_string._string.size() >= width) {
            return new_string;
        }
        if (new_string._bytecode.empty()) {
            auto size = new_string._string.size();
            if (size <= UINT8_MAX) {
                new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString8));
                new_string._bytecode.push_back(static_cast<uint8_t>(size));
            } else if (size <= UINT16_MAX) {
                new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString16));
                auto buf = static_cast<uint16_t>(size);
                new_string._bytecode.append(reinterpret_cast<char*>(&buf), 2);
            } else if (size <= UINT32_MAX) {
                new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString32));
                auto buf = static_cast<uint32_t>(size);
                new_string._bytecode.append(reinterpret_cast<char*>(&buf), 4);
            } else if (size <= UINT64_MAX) {
                new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString64));
                auto buf = static_cast<uint64_t>(size);
                new_string._bytecode.append(reinterpret_cast<char*>(&buf), 8);
            }
        }
        switch (align) {
            case LayoutAlign::Start: {
                auto space = width - new_string._string.size();
                if (space > 0) {
                    auto repeat = static_cast<uint16_t>(std::min(space, gap));
                    space -= repeat;
                    new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    new_string._bytecode.push_back(' ');
                    new_string._bytecode.append(reinterpret_cast<char*>(&repeat), 2);
                }
                if (space > 0) {
                    auto repeat = static_cast<uint16_t>(space);
                    new_string._bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
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
                    
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(' ');
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                    
                    bc.append(new_string._bytecode.begin(), new_string._bytecode.end());

                    repeat = space - repeat;

                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(' ');
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                } else {
                    auto repeat = static_cast<uint16_t>(space / 2 - gap);
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(decoration);
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                    space -= repeat;

                    repeat = gap;
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(' ');
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                    space -= repeat;
                    
                    bc.append(new_string._bytecode.begin(), new_string._bytecode.end());
                    
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(' ');
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                    space -= repeat;

                    repeat = static_cast<uint16_t>(space);

                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
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
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
                    bc.push_back(decoration);
                    bc.append(reinterpret_cast<char*>(&repeat), 2);
                }
                if (space > 0) {
                    auto repeat = static_cast<uint16_t>(space);
                    bc.push_back(static_cast<uint8_t>(StyleString::BytecodeRepeat16));
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
};

class StyleStringBuilder {
    std::ostringstream _buffer{};
    std::string _bytecode{};

public:
    StyleStringBuilder() { }
    StyleStringBuilder(const StyleStringBuilder&) = delete;
    StyleStringBuilder(StyleStringBuilder&&) noexcept = default;
    StyleStringBuilder& operator=(const StyleStringBuilder&) = delete;
    StyleStringBuilder& operator=(StyleStringBuilder&&) noexcept = default;

    template<typename T>
    StyleStringBuilder& operator <<(T&& value) {
        auto pos = _buffer.tellp();
        _buffer << std::forward<T>(value);
        auto end = _buffer.tellp();
        auto size = end - pos;
        if (size <= UINT8_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString8));
            _bytecode.push_back(static_cast<uint8_t>(size));
        } else if (size <= UINT16_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString16));
            auto buf = static_cast<uint16_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 2);
        } else if (size <= UINT32_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString32));
            auto buf = static_cast<uint32_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 4);
        } else if (size <= UINT64_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeString64));
            auto buf = static_cast<uint64_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 8);
        }
        return *this;
    }
    
    StyleStringBuilder& operator <<(::tui::style::SetStyle&& value) {
        auto style = value._style;
        _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeAttrSet));
        _bytecode.append(reinterpret_cast<char*>(&style), sizeof(style));
        return *this;
    }

    StyleStringBuilder& operator <<(::tui::style::SetColor&& value) {
        auto style = value._style;
        _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeAttrOn));
        _bytecode.append(reinterpret_cast<char*>(&style), sizeof(style));
        return *this;
    }

    StyleStringBuilder& operator <<(::tui::style::EnableStyle&& value) {
        auto style = value._style;
        _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeAttrOn));
        _bytecode.append(reinterpret_cast<char*>(&style), sizeof(style));
        return *this;
    }

    StyleStringBuilder& operator <<(::tui::style::DisableStyle&& value) {
        auto style = value._style;
        _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeAttrOff));
        _bytecode.append(reinterpret_cast<char*>(&style), sizeof(style));
        return *this;
    }

    StyleStringBuilder& operator <<(::tui::style::ResetStyle&& value) {
        auto style = 0;
        _bytecode.push_back(static_cast<uint8_t>(StyleString::BytecodeAttrSet));
        _bytecode.append(reinterpret_cast<char*>(&style), sizeof(style));
        return *this;
    }

    StyleString str() {
        return StyleString{ _buffer.str(), _bytecode };
    }

    StyleString release() {
        return { _buffer.str(), std::move(_bytecode) };
    }

    static
    void print_style_string(WINDOW* win, const StyleString& string, bool reverse=false) {
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
                switch(static_cast<StyleString::Bytecode>(code[0])) {
                    case StyleString::Bytecode::BytecodeNop:
                        str += 1;
                        code += 1;
                        break;
                    case StyleString::Bytecode::BytecodeString8: {
                        auto size = code[1];
                        auto space = size;
                        waddnstr(win, str, space);
                        str += size;
                        code += 2;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString16: {
                        uint16_t size = 0;
                        memcpy(&size, &code[1], 2);
                        auto space = size;
                        waddnstr(win, str, space);
                        str += size;
                        code += 3;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString32: {
                        uint32_t size = 0;
                        memcpy(&size, &code[1], 4);
                        auto space = size;
                        waddnstr(win, str, space);
                        str += size;
                        code += 5;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString64: {
                        uint64_t size = 0;
                        memcpy(&size, &code[1], 8);
                        auto space = size;
                        waddnstr(win, str, space);
                        str += size;
                        code += 7;
                        break;
                    }
                    case StyleString::BytecodeAttrOn: {
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
                    case StyleString::BytecodeAttrOff: {
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
                    case StyleString::BytecodeAttrSet: {
                        NCURSES_CH_T attrs = 0;
                        memcpy(&attrs, &code[1], sizeof(attrs));
                        if (reverse) {
                            attrs = (attrs & ~A_REVERSE) | (~attrs & A_REVERSE);
                        }
                        wattrset(win, attrs);
                        code += (1 + sizeof(attrs));
                        break;
                    }
                    case StyleString::BytecodeAttrClear: {
                        wattrset(win, default_attrs);
                        code += 1;
                        break;
                    }
                    case StyleString::BytecodeRepeat16: {
                        auto ch = code[1];
                        uint16_t count = 0;
                        memcpy(&count, &code[2], sizeof(count));
                        while(count) {
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
};

class TUI;

enum ContentProviderFlags {
    ContentProviderFlagAutoScrollTail = 1
};

struct ContentProvider {
    virtual void tui_show(size_t width) {};
    virtual void tui_hide() {};
    virtual bool tui_key(int key) { return false; };

    virtual std::string tui_name() = 0;
    virtual StyleString tui_title(size_t width) = 0;
    virtual size_t tui_count() = 0;
    virtual StyleString tui_item(size_t lineno, size_t width) = 0;

    virtual void tui_notify_changed();

    uint32_t flags() const {
        return _flags;
    }

    void set_flags(uint32_t flags) {
        _flags = flags;
    }

private:
    friend class TUI;
    TUI* _tui{nullptr};
    uint32_t _flags;
};

struct CommandHandler {
    virtual StyleString tui_prompt(size_t width) = 0;
    virtual void tui_run(const std::string& command) = 0;
    virtual bool tui_key(int key) { return false; }
};

class Editor {
    std::string _buffer {};
    size_t _cursor { 0 };
    size_t _height { 1 };

public:
    Editor()
    {
        _buffer.reserve(256);
    }

    void reset() {
        _buffer.clear();
        _cursor = 0;
        _height = 1;
    }

    const std::string& buffer() const
    {
        return _buffer;
    }

    size_t cursor() const
    {
        return _cursor;
    }

    size_t height() const
    {
        return _height;
    }

    void update(const std::string& buffer, size_t cursor)
    {
        _buffer = buffer;
        _cursor = cursor;
        if (_cursor > _buffer.size()) {
            _cursor = _buffer.size();
        }
    }

    void input(int key)
    {
        switch (key) {
        case KEY_LEFT:
            _cursor -= _cursor == 0 ? 0 : 1;
            break;
        case KEY_RIGHT:
            if (_cursor >= _buffer.size()) {
                _cursor = _buffer.size();
            } else {
                _cursor += 1;
            }
            break;
        case KEY_DC:
        case KEY_BACKSPACE:
        case 127:
            if (_cursor != 0) {
                _buffer.erase(_buffer.begin() + (_cursor - 1));
                _cursor -= 1;
            }
            break;
        case KEY_ENTER:
        case '\n':
            break;
        default:
            if (isascii(key)) {
                _buffer.insert(_cursor, 1, key);
                _cursor += 1;
            }
        }
    }
};

namespace detail {

struct ItemCache {
    StyleString _conetnt;
    size_t _height;
};

} // namespace detail

enum TUIFlags {
    TUIFlagColor = 1
};

class TUI {
    bool _exit { false };
    bool _editing { true };

    WINDOW* _win_title;
    WINDOW* _win_stage;
    WINDOW* _win_editor;

    WINDOW* _win_canvas;

    Editor _editor {};

    size_t _title_height { 1 };
    StyleString _title_string {};

    size_t _content_scroll_lines{0};
    size_t _content_cached_index{0};
    size_t _content_cached_lines{0};
    size_t _content_selected_index{0};
    std::vector<detail::ItemCache> _content_cached_items{};

    std::shared_ptr<CommandHandler> _handler {};
    std::shared_ptr<ContentProvider> _provider {};

public:
    TUI(int flags = 0)
    {
        initscr();
        cbreak();
        noecho();
        set_escdelay(0);

        if (has_colors() and flags & TUIFlagColor) {
            start_color();
            auto bg = COLOR_BLACK;
            init_pair(style::ColorNormal, COLOR_WHITE, bg);
            init_pair(style::ColorHighlight, bg, COLOR_WHITE);
            init_pair(style::ColorWarning, COLOR_YELLOW, bg);
            init_pair(style::ColorError, COLOR_RED, bg);
            init_pair(style::ColorInfo, COLOR_BLUE, bg);
            init_pair(style::ColorPrompt, COLOR_GREEN, bg);
            init_pair(style::ColorImportant, COLOR_WHITE, COLOR_RED);
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

    ~TUI()
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

    Editor& editor() { return _editor; }

    std::shared_ptr<ContentProvider> current_content_provider() {
        return _provider;
    }

    void attach(std::shared_ptr<CommandHandler>&& handler)
    {
        _handler = handler;
    }

    void show(std::shared_ptr<ContentProvider>&& provider)
    {
        show(provider);
    }

    void show(std::shared_ptr<ContentProvider>& provider)
    {
        if (_provider) {
            _provider->tui_hide();
            _provider->_tui = nullptr;
        }
        _provider = provider;

        if (_provider) {
            _provider->_tui = this;

            int width, height;
            getmaxyx(_win_canvas, height, width);
            _provider->tui_show(width);
            
            getmaxyx(_win_title, height, width);
            _title_string = _provider->tui_title(width);

            invalidate();
        }
    }

    void exit()
    {
        _exit = true;
    }

    int run()
    {
        int key;

        _exit = false;

        while (not _exit) {
            draw();
            key = wgetch(_win_editor);

            // ESC
            if (key == 27) {
                _editing = !_editing;
                if (not _editing) {
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
                continue;
            }

            if (_editing) {
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
                if (_provider == nullptr or not _provider->tui_key(key)) {
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

private:
    void list_mode_key(int key) {
        int screen_height, screen_width, offset = 0;
        getmaxyx(stdscr, screen_height, screen_width);
        const auto content_height = screen_height - _title_height - _editor.height();
        
        switch(key) {
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
            case '\n':
                _editing = true;
                invalidate();
                if (_handler && _provider) {
                    _handler->tui_run(_provider->tui_name() + " " + std::to_string(_content_selected_index));
                }
                break;
        }
    }
    void resize()
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

    void draw()
    {
        if (_provider) {
            werase(_win_title);
            wmove(_win_title, 0, 0);
            StyleStringBuilder::print_style_string(_win_title, _title_string, !_editing);
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

            if (_editing) {
                auto prompt = _handler->tui_prompt(width);

                StyleStringBuilder::print_style_string(_win_editor, prompt);

                wprintw(_win_editor, "%s ", _editor.buffer().c_str());
                wmove(_win_editor, 0, _editor.cursor() + prompt.size());
            } else {
                waddstr(_win_editor, "Press ESC enter edit mode");
            }
            wnoutrefresh(_win_editor);
        }

        doupdate();
    }

    void invalidate() {
        size_t screen_width, screen_height;
        getmaxyx(stdscr, screen_height, screen_width);

        const auto content_height = screen_height - _title_height - _editor.height();

        auto count = _provider->tui_count();

        _content_cached_items.clear();
        _content_cached_items.reserve(content_height);
        _content_cached_lines = 0;

        auto show_tail = (_provider->_flags & ContentProviderFlagAutoScrollTail) && _editing;

        if (count <= content_height) {
            _content_cached_index = 0;
        } else if (show_tail) {
            _content_cached_index = count - content_height;
            _content_scroll_lines = 0;
        }

        for (auto idx = 0; idx < std::min(content_height, count); ++idx) {
            detail::ItemCache item{};
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

        int y =  0;
        int idx = 0;
        for (auto& cache : _content_cached_items) {
            wmove(_win_canvas, y, 0);
            StyleStringBuilder::print_style_string(_win_canvas, cache._conetnt, 
                ((idx + _content_cached_index) == _content_selected_index) and not _editing);
            idx += 1;
            y += cache._height;
        }
    }

    friend void ContentProvider::tui_notify_changed();
};

template <typename T>
class VisibleVector : protected std::vector<T>, public ContentProvider {
    std::vector<T> _vector {};
    size_t _offset { 0 };
    size_t _width { 0 };

public:
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;

    using std::vector<T>::vector;
    using std::vector<T>::begin;
    using std::vector<T>::end;
    using std::vector<T>::size;
    using std::vector<T>::reserve;
    using std::vector<T>::at;

    void clear()
    {
        std::vector<T>::clear();
        tui_notify_changed();
    }

    template <typename Type>
    auto erase(Type&& value)
    {
        auto iter = std::vector<T>::erase(std::forward<Type>(value));
        tui_notify_changed();
        return iter;
    }

    template <typename Type>
    void push_back(Type&& value)
    {
        std::vector<T>::push_back(std::forward<Type>(value));
        tui_notify_changed();
    }

    template <typename Type>
    void emplace_back(Type&& value)
    {
        std::vector<T>::push_back(std::forward<Type>(value));
        tui_notify_changed();
    }

    void tui_show(size_t width) override
    {
        _width = width;
    }

    void tui_hide() override
    {
        _width = 0;
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString{"Visible Vector"};
    }

    size_t tui_count() override {
        return size();
    }

    StyleString tui_item(size_t lineno, size_t width) override
    {
        auto index = _offset + lineno;
        if (index >= this->size()) {
            return StyleString{};
        }
        return StyleString{this->at(_offset + lineno)};
    }
};

inline
void ContentProvider::tui_notify_changed() {
    if (_tui) {
        _tui->invalidate();
    }
}

class HistoryView : public VisibleVector<std::string> {
    ssize_t _index{-1};
    std::string _saved_buffer{};

public:
    HistoryView() {
        set_flags(ContentProviderFlagAutoScrollTail);
    }

    std::string tui_name() override {
        return "history";
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("History", width, 1, '=', LayoutAlign::Center);
    }

    void history_key(int key, Editor& editor) {
        switch(key) {
            case KEY_UP:
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

    void tui_notify_changed() override {
        VisibleVector<std::string>::tui_notify_changed();
        _index = -1;
    }
};

class MessageView;

class MessageStream {
    friend class MessageView;

    std::shared_ptr<MessageView> _view;
    StyleStringBuilder _builder{};

    MessageStream(std::shared_ptr<MessageView>& view)
    : _view(view)
    { }

    MessageStream(std::shared_ptr<MessageView>&& view)
    : _view(std::move(view))
    { }

public:
    ~MessageStream();
    MessageStream() { }
    MessageStream(const MessageStream&) = delete;
    MessageStream(MessageStream&&) noexcept = default;
    MessageStream& operator=(const MessageStream&) = delete;
    MessageStream& operator=(MessageStream&&) noexcept = default;

    template<typename T>
    MessageStream operator <<(T&& value) && {
        _builder << std::forward<T>(value);
        return std::move(*this);
    }
};

class MessageView : public VisibleVector<StyleString>, public std::enable_shared_from_this<MessageView>
{
    size_t _message_max;
public:
    MessageView(size_t message_max = 1000):_message_max(message_max) {
        set_flags(ContentProviderFlagAutoScrollTail);
    }
    
    std::string tui_name() override {
        return "message";
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("Message", width, 1, '=', LayoutAlign::Center);
    }

    MessageStream stream() {
        if (this->size() > _message_max) {
            auto half = _message_max / 2;
            auto iter = begin();
            while (size() > half) {
                iter = erase(iter);
            }
        }
        return {shared_from_this()};
    } 
};

inline
MessageStream::~MessageStream() {
    if (_view) {
        _view->emplace_back(_builder.str());
        _view.reset();
    }
}

} // namespace tui

#endif
