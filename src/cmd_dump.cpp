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
using namespace std::string_literals;

namespace mypower {

template<typename T>
struct DataNumber {
    VMAddress _addr{0};
    T _value;

    VMAddress address() {
        return _addr;
    }

    void value(std::ostringstream& oss) {
        oss << _value;
    }
};

template<typename T>
class DataView : public VisibleContainer<T> {
public:
    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Data", width, 1, '-', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        using namespace tui::attributes;

        return {};
    }
};

class Dump : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    Dump(Application& app)
        : Command(app)
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
        _options.add_options()("hexdump", po::bool_switch()->default_value(false), "hexdump");
        _options.add_options()("end,e", po::bool_switch()->default_value(false), "count is end address");
        _options.add_options()("force", po::bool_switch()->default_value(false), "Allow very long data");
        _options.add_options()("address,a", po::value<std::string>(), "start address");
        _options.add_options()("count,c", po::value<std::string>(), "count");
        _posiginal.add("address", 1);
        _posiginal.add("count", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "dump";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        // DumpArgs args;

        // try {
        //     if (opts.count("address")) {
        //         args._address = opts["address"].as<std::string>();
        //     }
        //     if (opts.count("count")) {
        //         args._count = opts["count"].as<std::string>();
        //     }
        //     args._count_as_end = opts["end"].as<bool>();
        //     args._type_bits |= opts["I8"].as<bool>() ? MatchTypeBitI8 : 0;
        //     args._type_bits |= opts["I16"].as<bool>() ? MatchTypeBitI16 : 0;
        //     args._type_bits |= opts["I32"].as<bool>() ? MatchTypeBitI32 : 0;
        //     args._type_bits |= opts["I64"].as<bool>() ? MatchTypeBitI64 : 0;
        //     args._type_bits |= opts["U8"].as<bool>() ? MatchTypeBitU8 : 0;
        //     args._type_bits |= opts["U16"].as<bool>() ? MatchTypeBitU16 : 0;
        //     args._type_bits |= opts["U32"].as<bool>() ? MatchTypeBitU32 : 0;
        //     args._type_bits |= opts["U64"].as<bool>() ? MatchTypeBitU64 : 0;
        // } catch (const std::exception& e) {
        //     message()
        //         << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
        //         << e.what();
        //     show();
        //     return;
        // }

        // if (args._address.empty() or args._count.empty()) {
        //     message() << "Usage: " << command << " [options] address count\n"
        //                             << _options;
        //     show();
        //     return;
        // }

        // if (not _app._process) {
        //     message()
        //         << SetColor(ColorError)
        //         << "Error:"
        //         << ResetStyle()
        //         << " Invalid target process, attach to target using the 'attach' command";
        //     show();
        //     return;
        // }
    }
};

static RegisterCommand<Dump> _Dump {};

} // namespace mypower
