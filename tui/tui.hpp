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
    void print_style_string(const StyleString& string, size_t width) {
        if (string._bytecode.empty()) {
            addnstr(string._string.c_str(), std::min(width, string._string.size()));
        } else {
            auto* str = string._string.data();
            auto* ends = str + string._string.size();
            auto* code = string._bytecode.data();
            auto* endc = code + string._bytecode.size();
            auto capacity = width;
            while (code < endc) {
                switch(static_cast<StyleString::Bytecode>(code[0])) {
                    case StyleString::Bytecode::BytecodeNop:
                        str += 1;
                        code += 1;
                        break;
                    case StyleString::Bytecode::BytecodeString8: {
                        auto size = code[1];
                        auto space = std::min((size_t)size, capacity);
                        addnstr(str, space);
                        str += size;
                        capacity -= space;
                        code += 2;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString16: {
                        uint16_t size = 0;
                        memcpy(&size, &code[1], 2);
                        auto space = std::min((size_t)size, capacity);
                        addnstr(str, space);
                        str += size;
                        capacity -= space;
                        code += 3;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString32: {
                        uint32_t size = 0;
                        memcpy(&size, &code[1], 4);
                        auto space = std::min((size_t)size, capacity);
                        addnstr(str, space);
                        str += size;
                        capacity -= space;
                        code += 5;
                        break;
                    }
                    case StyleString::Bytecode::BytecodeString64: {
                        uint64_t size = 0;
                        memcpy(&size, &code[1], 8);
                        auto space = std::min((size_t)size, capacity);
                        addnstr(str, space);
                        str += size;
                        capacity -= space;
                        code += 7;
                        break;
                    }
                    case StyleString::BytecodeAttrOn: {
                        NCURSES_CH_T attrs = 0;
                        memcpy(&attrs, &code[1], sizeof(attrs));
                        attron(attrs);
                        code += (1 + sizeof(attrs));
                        break;
                    }
                    case StyleString::BytecodeAttrOff: {
                        NCURSES_CH_T attrs = 0;
                        memcpy(&attrs, &code[1], sizeof(attrs));
                        attroff(attrs);
                        code += (1 + sizeof(attrs));
                        break;
                    }
                    case StyleString::BytecodeAttrSet: {
                        NCURSES_CH_T attrs = 0;
                        memcpy(&attrs, &code[1], sizeof(attrs));
                        attrset(attrs);
                        code += (1 + sizeof(attrs));
                        break;
                    }
                    case StyleString::BytecodeAttrClear: {
                        attrset(0);
                        code += 1;
                        break;
                    }
                    case StyleString::BytecodeRepeat16: {
                        auto ch = code[1];
                        uint16_t count = 0;
                        memcpy(&count, &code[2], sizeof(count));
                        while(count) {
                            addch(ch);
                            count -= 1;
                        }
                        code += 4;
                        break;
                    }
                }
            }
        }
    }
};

struct ContentProvider {
    virtual void tui_show(size_t width, size_t height) {};
    virtual void tui_hide() {};
    virtual void tui_key(int key) {};
    virtual void tui_scroll(ssize_t offset, size_t height) {};

    virtual StyleString tui_title(size_t width) = 0;
    virtual StyleString tui_draw(size_t lineno, size_t width) = 0;
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

enum TUIFlags {
    TUIFlagColor = 1
};

class TUI {
    bool _exit { false };
    bool _editing { true };
    Editor _editor {};

    size_t _title_height { 1 };
    StyleString _title_string {};

    std::shared_ptr<CommandHandler> _handler {};
    std::shared_ptr<ContentProvider> _provider {};

public:
    TUI(int flags = 0)
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
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
    }

    ~TUI()
    {
        _handler.reset();
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
        }
        _provider = provider;

        if (_provider) {
            size_t width, height;
            getmaxyx(stdscr, height, width);
            _provider->tui_show(width, height - _title_height - _editor.height());
            _title_string = _provider->tui_title(width);
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
            key = getch();
            if (_handler == nullptr or not _handler->tui_key(key)) {
                if (_editing) {
                    if (key == KEY_ENTER or key == '\n') {
                        if (_handler) {
                            _handler->tui_run(_editor.buffer());
                            _editor.reset();
                        }
                    } else {
                        _editor.input(key);
                    }
                } else {
                    if (_provider) {
                        _provider->tui_key(key);
                    }
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
    void resize()
    {
        auto provider = _provider;
        show(provider);
        this->draw();
    }

    void draw()
    {
        size_t screen_width, screen_height;
        getmaxyx(stdscr, screen_height, screen_width);

        size_t content_height = screen_height - _editor.height();
        size_t content_width = screen_width;

        erase();

        if (_provider) {
            move(0, 0);
            StyleStringBuilder::print_style_string(_title_string, screen_width);
            
            for (size_t y = 1; y < content_height; ++y) {
                StyleString line = _provider->tui_draw(y - 1, content_width);
                move(y, 0);
                StyleStringBuilder::print_style_string(line, screen_width);
            }
        }

        auto prompt = _handler->tui_prompt(screen_width);

        wmove(stdscr, content_height, 0);

        StyleStringBuilder::print_style_string(prompt, screen_width);

        printw("%s", _editor.buffer().c_str());
        wmove(stdscr, content_height, _editor.cursor() + prompt.size());

        refresh();
    }
};

enum VisibleVectorFlags {
    VisibleVectorFlagTail = 1
};

template <typename T>
class VisibleVector : protected std::vector<T>, public ContentProvider {
    std::vector<T> _vector {};
    size_t _offset { 0 };
    size_t _width { 0 };
    size_t _height { 0 };
    uint32_t _flags { 0 };

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
        update_offset();
    }

    template <typename Type>
    auto erase(Type&& value)
    {
        auto iter = std::vector<T>::erase(std::forward<Type>(value));
        update_offset();
        return iter;
    }

    template <typename Type>
    void push_back(Type&& value)
    {
        std::vector<T>::push_back(std::forward<Type>(value));
        update_offset();
    }

    template <typename Type>
    void emplace_back(Type&& value)
    {
        std::vector<T>::push_back(std::forward<Type>(value));
        update_offset();
    }

    void set_flags(uint32_t flags)
    {
        _flags = flags;
    }

    uint32_t flags() const
    {
        return _flags;
    }

    void tui_show(size_t width, size_t height) override
    {
        _width = width;
        _height = height;
        update_offset();
    }

    void tui_hide() override
    {
        _width = 0;
        _height = 0;
    }

    void tui_scroll(ssize_t offset, size_t height) override
    {
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString{"Visible Vector"};
    }

    StyleString tui_draw(size_t lineno, size_t width) override
    {
        auto index = _offset + lineno;
        if (index >= this->size()) {
            return StyleString{};
        }
        return StyleString{this->at(_offset + lineno)};
    }

private:
    void update_offset()
    {
        if (_width == 0 or _height == 0) {
            return;
        }

        if (this->size() > _height) {
            if (_flags & VisibleVectorFlagTail) {
                _offset = this->size() - _height;
            } else {
                _offset = 0;
            }
        } else {
            _offset = 0;
        }
    }
};

class HistoryView : public VisibleVector<std::string> {
public:
    HistoryView() {
        set_flags(VisibleVectorFlagTail);
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("History", width, 1, '=', LayoutAlign::Center);
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
        set_flags(VisibleVectorFlagTail);
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
