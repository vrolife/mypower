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

#define LIMIT_IN_BYTES 1024 * 1024

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

struct RefreshInterface {
    virtual void refresh() = 0;
};

template <typename T>
class DataViewContinuous : public VisibleContainer<T>, public RefreshInterface {
    uintptr_t _address;
    char _mode;
    int _auto_refresh { -1 };
    std::shared_ptr<Process>& _process;

    int _unity3d_object { -1 };
    int _unity3d_class { -1 };
    int _cstring_index { -1 };

public:
    DataViewContinuous(std::shared_ptr<Process>& process, uintptr_t address, char mode = 'h')
        : _process(process)
        , _address(address)
        , _mode{mode}
    {
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Data", width, 1, '-', LayoutAlign::Center);
    }

    void show_unity32_object_type(AttributedStringBuilder& builder, VMAddress vmaddr)
    {
        uintptr_t class_ptr { 0 };

        if (_process->read(vmaddr, &class_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return;
        }

        return show_unity32_class_name(builder, VMAddress{class_ptr});
    }

    void show_unity32_class_name(AttributedStringBuilder& builder, VMAddress class_ptr)
    {
        uintptr_t name_ptr { 0 };

        if (_process->read(VMAddress { class_ptr + 2 * sizeof(uintptr_t) }, &name_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return;
        }

        show_cstring(builder, VMAddress{name_ptr});
    }

    void show_cstring(AttributedStringBuilder& builder, VMAddress ptr) {
        std::array<char, 32> buffer {};
        if (_process->read(ptr, buffer.data(), buffer.size()) == buffer.size()) {
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

        if constexpr (std::is_integral<T>::value) {
            switch(_mode) {
                case 'h':
                    builder << "0x" << std::hex << data;
                    break;
                case 'd':
                    builder << std::dec << data;
                    break;
                case 'o':
                    builder << "0" << std::oct << data;
                    break;
            }
            
        } else if constexpr (std::is_floating_point<T>::value) {
            builder << std::dec << data;

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
            if (index == _unity3d_object and _mode == 'h' and sizeof(T) == sizeof(uintptr_t)) {
                builder << " | Object: " << SetColor(ColorInfo);
                show_unity32_object_type(builder, VMAddress{data});
                builder << ResetStyle{};
            }
            
            if (index == _unity3d_class and _mode == 'h' and sizeof(T) == sizeof(uintptr_t)) {
                builder << " | Class: " << SetColor(ColorInfo);
                show_unity32_class_name(builder, VMAddress{data});
                builder << ResetStyle{};
            }

            if (index == _cstring_index and _mode == 'h' and sizeof(T) == sizeof(uintptr_t)) {
                builder << " | String: " << SetColor(ColorInfo);
                show_cstring(builder, VMAddress{data});
                builder << ResetStyle{};
            }
        }

        return builder.release();
    }

    void refresh() override
    {
        _process->read(VMAddress { _address }, this->data(), this->size() * sizeof(T));
    }

    bool tui_key(size_t index, int key) override
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
        case 'o':
            _unity3d_object = index;
            this->tui_notify_changed();
            return true;
        case 'c':
            _unity3d_class = index;
            this->tui_notify_changed();
            return true;
        case 's':
            _cstring_index = index;
            this->tui_notify_changed();
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

    // std::string tui_select(size_t index) override
    // {
    //     return {};
    // }
};

template <typename T>
static auto create_number_view(std::shared_ptr<Process>& process, uintptr_t addr, uintptr_t count, size_t limit_in_bytes, char mode)
{
    auto view = std::make_shared<DataViewContinuous<T>>(process, addr, mode);

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
        _options.add_options()("FLOAT,f", po::bool_switch()->default_value(false), "float");
        _options.add_options()("DOUBLE,d", po::bool_switch()->default_value(false), "double");
        _options.add_options()("HEX,hex", po::bool_switch()->default_value(false), "hexdump");
        _options.add_options()("end,e", po::bool_switch()->default_value(false), "count is end address");
        _options.add_options()("refresh,r", po::bool_switch()->default_value(false), "refresh current view");
        _options.add_options()("limit_in_bytes", po::value<size_t>()->default_value(LIMIT_IN_BYTES), "Limit");
        _options.add_options()("address,a", po::value<std::string>(), "start address");
        _options.add_options()("count,c", po::value<std::string>(), "count");
        _posiginal.add("address", 1);
        _posiginal.add("count", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "view" or memcmp(command.c_str(), "x/",2) == 0;
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        using namespace tui::attributes;

        std::string address_expr {};
        std::string count_expr {};
        uint32_t type { 0 };
        bool count_as_end(false);
        size_t limit_in_bytes { LIMIT_IN_BYTES };
        char mode{'h'};

        if (memcmp(command.c_str(), "x/", 2) == 0) {
            char width{'w'};
            char format{'x'};
            auto iter = command.begin() + 2;
            while(iter != command.end()) {
                if (std::isdigit(*iter)) {
                    while(std::isdigit(*iter)) {
                        count_expr.push_back(*iter);
                        ++iter;
                    }
                    continue;
                }
                switch(*iter) {
                    case 'x':
                        mode = 'h';
                        format = *iter;
                        break;
                    case 'o':
                        mode = 'o';
                        format = *iter;
                        break;
                    case 'd':
                    case 'u':
                        mode = 'd';
                        format = *iter;
                        break;
                    case 'f':
                    case 's':
                        format = *iter;
                        break;
                    case 'b':
                    case 'h':
                    case 'w':
                    case 'g':
                        width = *iter;
                        break;
                    default:
                        message()
                            << EnableStyle(AttrUnderline) << SetColor(ColorError) 
                            << "Error: " << ResetStyle()
                            << "Unsupported option: " << *iter;
                        show();
                        return;
                }
                ++iter;
            }

            if (format == 'd' or format == 'o') {
                switch(width) {
                    case 'b': type = MatchTypeBitI8; break;
                    case 'h': type = MatchTypeBitI16; break;
                    case 'w': type = MatchTypeBitI32; break;
                    case 'g': type = MatchTypeBitI64; break;
                }
            } else if (format == 'u' or format == 'x') {
                switch(width) {
                    case 'b': type = MatchTypeBitU8; break;
                    case 'h': type = MatchTypeBitU16; break;
                    case 'w': type = MatchTypeBitU32; break;
                    case 'g': type = MatchTypeBitU64; break;
                }
            } else if (format == 'f') {
                switch(width) {
                    case 'b':
                    case 'h':
                        message()
                            << EnableStyle(AttrUnderline) << SetColor(ColorError) 
                            << "Error: " << ResetStyle()
                            << "Unsupported floating point width: " << width;
                        show();
                        return;
                    case 'w': type = MatchTypeBitFLOAT; break;
                    case 'g': type = MatchTypeBitDOUBLE; break;
                }
            } else if (format == 's') {
                type = MatchTypeBitBYTES;
            }

            if (arguments.empty()) {
                message()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) 
                    << "Error: " << ResetStyle()
                    << "Missing address expression";
                show();
                return;
            }

            address_expr = arguments.at(0);

            
        } else {
            PROGRAM_OPTIONS();
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
                type |= opts["FLOAT"].as<bool>() ? MatchTypeBitFLOAT : 0;
                type |= opts["DOUBLE"].as<bool>() ? MatchTypeBitDOUBLE : 0;
                type |= opts["HEX"].as<bool>() ? MatchTypeBitBYTES : 0;
            } catch (const std::exception& e) {
                message()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                    << e.what();
                show();
                return;
            }

            if (count_expr.empty() or opts.count("help")) {
                message() << "Usage: " << command << " [options] address count\n"
                        << _options;
                show();
                return;
            }
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

        if (address_expr.empty()) {
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
        view = create_number_view<type##t>(_app._process, addr_number_ast->_value, count_number_ast->_value, limit_in_bytes, mode); \
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
