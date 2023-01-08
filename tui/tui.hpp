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
    StyleString layout(StyleString& string, size_t width, size_t gap, int decoration, LayoutAlign align);

    const std::string& string() const {
        return _string;
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

    static void print_style_string(WINDOW* win, const StyleString& string, bool reverse=false);
};

class TUI;

enum ContentProviderFlags {
    ContentProviderFlagAutoScrollTail = 1
};

struct ContentProvider {
    virtual ~ContentProvider() = default;

    virtual void tui_show(size_t width) {};
    virtual void tui_hide() {};
    virtual bool tui_key(int key) { return false; };
    virtual std::string tui_select(size_t index) { return {}; }

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
    virtual ~CommandHandler() = default;
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
    TUI(int flags = 0);
    ~TUI();

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

    int run();

private:
    void list_mode_key(int key);

    void resize();

    void draw();

    void invalidate();

    friend void ContentProvider::tui_notify_changed();
};

template <typename T, typename C=std::vector<T>>
class VisibleContainer : protected C, public ContentProvider {
    C _vector {};

public:
    typedef typename C::iterator iterator;
    typedef typename C::const_iterator const_iterator;

    using C::C;
    using C::begin;
    using C::end;
    using C::size;
    using C::reserve;
    using C::at;

    void clear()
    {
        C::clear();
        tui_notify_changed();
    }

    template <typename Type>
    auto erase(Type&& value)
    {
        auto iter = C::erase(std::forward<Type>(value));
        tui_notify_changed();
        return iter;
    }

    template <typename Type>
    void push_back(Type&& value)
    {
        C::push_back(std::forward<Type>(value));
        tui_notify_changed();
    }

    template <typename Type>
    void emplace_back(Type&& value)
    {
        C::push_back(std::forward<Type>(value));
        tui_notify_changed();
    }

    size_t tui_count() override {
        return size();
    }
};

class HistoryView : public VisibleContainer<std::string> {
    ssize_t _index{-1};
    std::string _saved_buffer{};

public:
    HistoryView();

    StyleString tui_title(size_t width) override;

    void history_key(int key, Editor& editor);

    void tui_notify_changed() override;

    StyleString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return StyleString{};
        }
        return StyleString{this->at(index)};
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

class MessageView : public VisibleContainer<StyleString>, public std::enable_shared_from_this<MessageView>
{
    size_t _message_max;
public:
    MessageView(size_t message_max = 1000):_message_max(message_max) {
        set_flags(ContentProviderFlagAutoScrollTail);
    }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("Message", width, 1, '=', LayoutAlign::Center);
    }
    
    StyleString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return StyleString{};
        }
        return StyleString{this->at(index)};
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
