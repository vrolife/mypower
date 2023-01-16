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

#include <cassert>
#include <cstring>

#include <algorithm>

#include <ncurses/ncurses.h>

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace tui {
class AttributedStringBuilder;

namespace attributes {
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
        friend class ::tui::AttributedStringBuilder;
        NCURSES_CH_T _attribute;

    public:
        SetStyle(NCURSES_CH_T attribute)
            : _attribute(attribute)
        {
        }
    };

    class SetColor {
        friend class ::tui::AttributedStringBuilder;
        NCURSES_CH_T _attribute;

    public:
        SetColor(Color color)
            : _attribute(COLOR_PAIR(color))
        {
        }
    };

    class EnableStyle {
        friend class ::tui::AttributedStringBuilder;
        NCURSES_CH_T _attribute;

    public:
        EnableStyle(NCURSES_CH_T attribute)
            : _attribute(attribute)
        {
        }
    };

    class DisableStyle {
        friend class ::tui::AttributedStringBuilder;
        NCURSES_CH_T _attribute;

    public:
        DisableStyle(NCURSES_CH_T attribute)
            : _attribute(attribute)
        {
        }
    };

    struct ResetStyle { };

} // namespace attribute

enum class LayoutAlign {
    Start,
    Center,
    End
};

class AttributedString {
    friend class AttributedStringBuilder;

    std::string _string {};
    std::string _bytecode {};

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

    AttributedString(std::string&& string, std::string&& bytecode)
        : _string(std::move(string))
        , _bytecode(std::move(bytecode))
    {
    }

    AttributedString(std::string&& string, std::string& bytecode)
        : _string(std::move(string))
        , _bytecode(bytecode)
    {
    }

public:
    AttributedString() = default;

    AttributedString(const std::string& string)
        : _string(string)
    {
    }

    AttributedString(const AttributedString&) = default;
    AttributedString(AttributedString&&) noexcept = default;
    AttributedString& operator=(const AttributedString&) = default;
    AttributedString& operator=(AttributedString&&) noexcept = default;

    operator std::string&()
    {
        return _string;
    }

    operator const std::string&() const
    {
        return _string;
    }

    size_t calc_height_slow() const
    {
        return std::count(_string.begin(), _string.end(), '\n') + 1;
    }

    size_t size() const { return _string.size(); }

    size_t empty() const { return _string.empty(); }

    bool attribute() const
    {
        return not _bytecode.empty();
    }

    static AttributedString layout(AttributedString&& string, size_t width, size_t gap, int decoration, LayoutAlign align)
    {
        return layout(string, width, gap, decoration, align);
    }

    static AttributedString layout(std::string&& string, size_t width, size_t gap, int decoration, LayoutAlign align)
    {
        return layout(string, width, gap, decoration, align);
    }

    static AttributedString layout(AttributedString& string, size_t width, size_t gap, int decoration, LayoutAlign align);

    const std::string& string() const
    {
        return _string;
    }
};

class AttributedStringBuilder {
    std::ostringstream _buffer {};
    std::string _bytecode {};

public:
    AttributedStringBuilder() { }
    AttributedStringBuilder(const AttributedStringBuilder&) = delete;
    AttributedStringBuilder(AttributedStringBuilder&&) noexcept = default;
    AttributedStringBuilder& operator=(const AttributedStringBuilder&) = delete;
    AttributedStringBuilder& operator=(AttributedStringBuilder&&) noexcept = default;

    template <typename T>
    AttributedStringBuilder& operator<<(T&& value)
    {
        stream([&](std::ostringstream& stream) {
            _buffer << std::forward<T>(value);
        });
        return *this;
    }

    template <typename T>
    void stream(T&& callback)
    {
        auto pos = _buffer.tellp();
        callback(_buffer);
        auto end = _buffer.tellp();
        auto size = end - pos;
        if (size <= UINT8_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString8));
            _bytecode.push_back(static_cast<uint8_t>(size));
        } else if (size <= UINT16_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString16));
            auto buf = static_cast<uint16_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 2);
        } else if (size <= UINT32_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString32));
            auto buf = static_cast<uint32_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 4);
        } else if (size <= UINT64_MAX) {
            _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeString64));
            auto buf = static_cast<uint64_t>(size);
            _bytecode.append(reinterpret_cast<char*>(&buf), 8);
        }
    }

    AttributedStringBuilder& operator<<(::tui::attributes::SetStyle&& value)
    {
        auto attribute = value._attribute;
        _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeAttrSet));
        _bytecode.append(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        return *this;
    }

    AttributedStringBuilder& operator<<(::tui::attributes::SetColor&& value)
    {
        auto attribute = value._attribute;
        _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeAttrOn));
        _bytecode.append(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        return *this;
    }

    AttributedStringBuilder& operator<<(::tui::attributes::EnableStyle&& value)
    {
        auto attribute = value._attribute;
        _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeAttrOn));
        _bytecode.append(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        return *this;
    }

    AttributedStringBuilder& operator<<(::tui::attributes::DisableStyle&& value)
    {
        auto attribute = value._attribute;
        _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeAttrOff));
        _bytecode.append(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        return *this;
    }

    AttributedStringBuilder& operator<<(::tui::attributes::ResetStyle&& value)
    {
        auto attribute = 0;
        _bytecode.push_back(static_cast<uint8_t>(AttributedString::BytecodeAttrSet));
        _bytecode.append(reinterpret_cast<char*>(&attribute), sizeof(attribute));
        return *this;
    }

    AttributedString str()
    {
        return AttributedString { _buffer.str(), _bytecode };
    }

    AttributedString release()
    {
        return { _buffer.str(), std::move(_bytecode) };
    }

    static void print_attributed_string(WINDOW* win, const AttributedString& string, bool reverse = false);
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

    virtual AttributedString tui_title(size_t width) = 0;
    virtual size_t tui_count() = 0;
    virtual AttributedString tui_item(size_t lineno, size_t width) = 0;

    virtual void tui_notify_changed();

    uint32_t flags() const
    {
        return _flags;
    }

    void set_flags(uint32_t flags)
    {
        _flags = flags;
    }

private:
    friend class TUI;
    TUI* _tui { nullptr };
    uint32_t _flags;
};

struct CommandHandler {
    virtual ~CommandHandler() = default;
    virtual AttributedString tui_prompt(size_t width) = 0;
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

    void reset()
    {
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
        AttributedString _conetnt;
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
    AttributedString _title_string {};

    size_t _content_scroll_lines { 0 };
    size_t _content_cached_index { 0 };
    size_t _content_cached_lines { 0 };
    size_t _content_selected_index { 0 };
    std::vector<detail::ItemCache> _content_cached_items {};

    std::shared_ptr<CommandHandler> _handler {};
    std::shared_ptr<ContentProvider> _provider {};

public:
    TUI(int flags = 0);
    ~TUI();

    Editor& editor() { return _editor; }

    std::shared_ptr<ContentProvider> current_content_provider()
    {
        return _provider;
    }

    void update_title();

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

template <typename T, typename C = std::vector<T>>
class VisibleContainer : protected C, public ContentProvider {
    C _vector {};

public:
    typedef typename C::iterator iterator;
    typedef typename C::const_iterator const_iterator;

    using C::at;
    using C::begin;
    using C::C;
    using C::end;
    using C::reserve;
    using C::size;

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

    size_t tui_count() override
    {
        return size();
    }
};

class HistoryView : public VisibleContainer<std::string> {
    ssize_t _index { -1 };
    std::string _saved_buffer {};

public:
    HistoryView();

    AttributedString tui_title(size_t width) override;

    void history_key(int key, Editor& editor);

    void tui_notify_changed() override;

    AttributedString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return AttributedString {};
        }
        return AttributedString { this->at(index) };
    }
};

class MessageView;

class MessageStream {
    friend class MessageView;

    std::shared_ptr<MessageView> _view;
    AttributedStringBuilder _builder {};

    MessageStream(std::shared_ptr<MessageView>& view)
        : _view(view)
    {
    }

    MessageStream(std::shared_ptr<MessageView>&& view)
        : _view(std::move(view))
    {
    }

public:
    ~MessageStream();
    MessageStream() { }
    MessageStream(const MessageStream&) = delete;
    MessageStream(MessageStream&&) noexcept = default;
    MessageStream& operator=(const MessageStream&) = delete;
    MessageStream& operator=(MessageStream&&) noexcept = default;

    template <typename T>
    MessageStream operator<<(T&& value) &&
    {
        _builder << std::forward<T>(value);
        return std::move(*this);
    }
};

class MessageView : public VisibleContainer<AttributedString>, public std::enable_shared_from_this<MessageView> {
    size_t _message_max;

public:
    MessageView(size_t message_max = 1000)
        : _message_max(message_max)
    {
        set_flags(ContentProviderFlagAutoScrollTail);
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Message", width, 1, '=', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        if (index >= this->size()) {
            return AttributedString {};
        }
        return AttributedString { this->at(index) };
    }

    MessageStream stream()
    {
        if (this->size() > _message_max) {
            auto half = _message_max / 2;
            auto iter = begin();
            while (size() > half) {
                iter = erase(iter);
            }
        }
        return { shared_from_this() };
    }
};

inline MessageStream::~MessageStream()
{
    if (_view) {
        _view->emplace_back(_builder.str());
        _view.reset();
    }
}

} // namespace tui

#endif
