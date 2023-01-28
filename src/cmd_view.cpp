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
#include <bitset>
#include <boost/program_options.hpp>

#include "mathexpr.hpp"
#include "mypower.hpp"
#include "scanner.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

struct RefreshInterface {
    virtual void refresh() = 0;
};

template <typename T>
class DataViewContinuous : public VisibleContainer<T>, public RefreshInterface {
    uintptr_t _address {};
    char _mode { 'd' };
    int _auto_refresh { -1 };
    std::shared_ptr<Process>& _process;

    int _unity3d_class { -1 };

public:
    DataViewContinuous(std::shared_ptr<Process>& process, uintptr_t address)
        : _process(process)
        , _address(address)
    {
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Data", width, 1, '-', LayoutAlign::Center);
    }

    void show_unity32_class_name(AttributedStringBuilder& builder, VMAddress vmaddr)
    {
        uintptr_t class_ptr { 0 };
        uintptr_t name_ptr { 0 };

        if (_process->read(vmaddr, &class_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return;
        }

        if (_process->read(VMAddress { class_ptr + 2 * sizeof(uintptr_t) }, &name_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return;
        }

        std::array<char, 32> buffer {};
        if (_process->read(VMAddress { name_ptr }, buffer.data(), buffer.size()) == buffer.size()) {
            builder << " | ";
            for (auto ch : buffer) {
                if (ch == 0) {
                    break;
                }
                if (ch >= 32 and ch <= 126) {
                    builder << ch;
                } else {
                    builder << ".";
                }
            }
        }
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        using namespace tui::attributes;
        AttributedStringBuilder builder {};
        auto& data = this->at(index);
        builder << "0x" << std::hex << (index * sizeof(T) + _address) << ": ";

        if constexpr (std::is_floating_point<T>::value or std::is_integral<T>::value) {
            if (_mode == 'h') {
                builder << "0x" << std::hex << data;
            } else {
                builder << std::dec << data;
            }
        } else {
            for (auto ch : data) {
                builder << std::setw(2) << std::setfill('0') << std::hex << (int)ch << " ";
            }
            builder << "| ";
            for (auto ch : data) {
                if (ch >= 32 and ch <= 126) {
                    builder << ch;
                } else {
                    builder << ".";
                }
            }
        }

        if constexpr (std::is_same<T, uintptr_t>::value) {
            if (index == _unity3d_class and _mode == 'h' and sizeof(T) == sizeof(uintptr_t)) {
                show_unity32_class_name(builder, VMAddress{data});
            }
        }

        return builder.release();
    }

    void refresh() override
    {
        _process->read(VMAddress { _address }, this->data(), this->size() * sizeof(T));
    }

    bool tui_key(int key) override
    {
        switch (key) {
        case 'h':
            _mode = 'h';
            this->tui_notify_changed();
            return true;
        case 'd':
            _mode = 'd';
            this->tui_notify_changed();
            return true;
        case 'r':
            this->refresh();
            this->tui_notify_changed();
            return true;
        case 'a':
            _auto_refresh = 1000;
            return true;
        case 'A':
            _auto_refresh = -1;
            return true;
        }
        return false;
    }

    bool tui_show(size_t width) override
    {
        return true;
    }

    int tui_timeout() override
    {
        this->refresh();
        this->tui_notify_changed();
        return _auto_refresh;
    }

    std::string tui_select(size_t index) override
    {
        _unity3d_class = index;
        return {};
    }
};

template <typename T>
static auto create_number_view(std::shared_ptr<Process>& process, uintptr_t addr, uintptr_t count, size_t limit_in_bytes)
{
    auto view = std::make_shared<DataViewContinuous<T>>(process, addr);

    size_t size_in_bytes = count * sizeof(T);
    if (size_in_bytes > limit_in_bytes) {
        throw std::out_of_range("Data size limited to "s + std::to_string(limit_in_bytes) + " Bytes");
    }

    view->resize(count);
    view->refresh();

    return view;
}

static auto create_bytes_view(std::shared_ptr<Process>& process, uintptr_t addr, uintptr_t count, size_t limit_in_bytes)
{
    constexpr size_t row_size = 16;
    typedef std::array<uint8_t, row_size> T;

    auto view = std::make_shared<DataViewContinuous<T>>(process, addr);

    if (count > limit_in_bytes) {
        throw std::out_of_range("Data size limited to "s + std::to_string(limit_in_bytes) + " Bytes");
    }

    auto rows = count / row_size;

    view->resize(rows);
    view->refresh();

    return view;
}

class CommandView : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandView(Application& app)
        : Command(app, "view")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("I64,q", po::bool_switch()->default_value(false), "64 bit integer");
        _options.add_options()("I32,i", po::bool_switch()->default_value(false), "32 bit integer");
        _options.add_options()("I16,h", po::bool_switch()->default_value(false), "16 bit integer");
        _options.add_options()("I8,b", po::bool_switch()->default_value(false), "8 bit integer");
        _options.add_options()("U64,Q", po::bool_switch()->default_value(false), "64 bit unsigned integer");
        _options.add_options()("U32,I", po::bool_switch()->default_value(false), "32 bit unsigned integer");
        _options.add_options()("U16,H", po::bool_switch()->default_value(false), "16 bit unsigned integer");
        _options.add_options()("U8,B", po::bool_switch()->default_value(false), "8 bit unsigned integer");
        _options.add_options()("HEX,hex", po::bool_switch()->default_value(false), "hexdump");
        _options.add_options()("end,e", po::bool_switch()->default_value(false), "count is end address");
        _options.add_options()("refresh,r", po::bool_switch()->default_value(false), "refresh current view");
        _options.add_options()("limit_in_bytes", po::value<size_t>()->default_value(1024 * 1024), "Limit");
        _options.add_options()("address,a", po::value<std::string>(), "start address");
        _options.add_options()("count,c", po::value<std::string>(), "count");
        _posiginal.add("address", 1);
        _posiginal.add("count", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "view";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        std::string address_expr {};
        std::string count_expr {};
        uint32_t type { 0 };
        bool count_as_end(false);
        size_t limit_in_bytes { 0 };

        try {
            if (opts.count("address")) {
                address_expr = opts["address"].as<std::string>();
            }
            if (opts.count("count")) {
                count_expr = opts["count"].as<std::string>();
            }
            count_as_end = opts["end"].as<bool>();
            limit_in_bytes = opts["limit_in_bytes"].as<size_t>();
            type |= opts["I8"].as<bool>() ? MatchTypeBitI8 : 0;
            type |= opts["I16"].as<bool>() ? MatchTypeBitI16 : 0;
            type |= opts["I32"].as<bool>() ? MatchTypeBitI32 : 0;
            type |= opts["I64"].as<bool>() ? MatchTypeBitI64 : 0;
            type |= opts["U8"].as<bool>() ? MatchTypeBitU8 : 0;
            type |= opts["U16"].as<bool>() ? MatchTypeBitU16 : 0;
            type |= opts["U32"].as<bool>() ? MatchTypeBitU32 : 0;
            type |= opts["U64"].as<bool>() ? MatchTypeBitU64 : 0;
            type |= opts["HEX"].as<bool>() ? MatchTypeBitBYTES : 0;
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        if (not _app._process) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Invalid target process, attach to target using the 'attach' command";
            show();
            return;
        }

        if (address_expr.empty() or count_expr.empty() or opts.count("help")) {
            message() << "Usage: " << command << " [options] address count\n"
                      << _options;
            show();
            return;
        }

        if (type == 0) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Data type not specified";
            show();
            return;
        }

        if (__builtin_clz(type) == __builtin_clz(type - 1)) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Multiple data types specified";
            show();
            return;
        }

        auto addr_ast = mathexpr::parse(address_expr);
        auto addr_number_ast = dynamic_cast<mathexpr::ASTNumber*>(addr_ast.get());
        if (addr_ast == nullptr) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Unsupported address expression";
            show();
            return;
        }

        auto count_ast = mathexpr::parse(count_expr);
        auto count_number_ast = dynamic_cast<mathexpr::ASTNumber*>(count_ast.get());
        if (count_ast == nullptr) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Unsupported count expression";
            show();
            return;
        }

        if (count_as_end) {
            count_number_ast->_value -= addr_number_ast->_value;
        }
        try {
            std::shared_ptr<ContentProvider> view {};

            switch (type) {
#define __NUMBER_VIEW(t)                                                                                                      \
    case MatchTypeBit##t:                                                                                                     \
        view = create_number_view<type##t>(_app._process, addr_number_ast->_value, count_number_ast->_value, limit_in_bytes); \
        break;

                MATCH_TYPES_NUMBER(__NUMBER_VIEW)
#undef __NUMBER_VIEW
            case MatchTypeBitBYTES:
                view = create_bytes_view(_app._process, addr_number_ast->_value, count_number_ast->_value, limit_in_bytes);
                break;
            }

            show(view);

        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }
    }
};

static RegisterCommand<CommandView> _Dump {};

} // namespace mypower
