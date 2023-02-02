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
#include <regex>
#include <boost/program_options.hpp>

#include "mathexpr.hpp"
#include "scanner.hpp"
#include "mypower.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {


class RegionListView : public VisibleContainer<ssize_t> {
    VMRegion::ListType _regions;

public:
    RegionListView(VMRegion::ListType&& regions)
    :_regions(std::move(regions))
    {
    }

    ~RegionListView() override { }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Regions", width, 1, '-', LayoutAlign::Center);
    }
    
    size_t tui_count() override
    {
        return _regions.size();
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        using namespace tui::attributes;
        AttributedStringBuilder builder {};
        builder.stream([&](std::ostringstream& oss) { _regions.at(index).string(oss); });
        return builder.release();
    }
};

class CommandRegion : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandRegion(Application& app)
        : Command(app, "region")
    {
        _options.add_options()("regex,r", po::bool_switch()->default_value(false), "regex filter");
        _options.add_options()("filter", po::value<std::string>(), "filter");
        _posiginal.add("filter", 1);
    }

    std::string complete(const std::string& input) override
    {
        if ("region"s.find(input) == 0) {
            return "region";
        }
        return {};
    }

    bool match(const std::string& command) override
    {
        return command == "region";
    }

    void show_short_help() override {
        message() << "region\t\t\tShow all regions";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        std::string filter {};

        try {
            if (opts.count("filter")) {
                filter = opts["filter"].as<std::string>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        if (opts.count("help")) {
            message() << "Usage: " << command << " [options] filter\n"
                      << _options;
            show();
            return;
        }
        
        if (not _app._process) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Invalid target process: attach to target using the 'attach' command";
            show();
            return;
        }

        if (not opts["regex"].as<bool>()) {
            filter = ".*"s + filter + ".*"s;
        }

        auto regions = _app._process->get_memory_regions();

        if (not filter.empty()) {
            try {
                std::regex regexp { filter };

                for(auto iter = regions.begin(); iter != regions.end();) {
                    if (not std::regex_match(iter->_file, regexp)) {
                        iter = regions.erase(iter);
                    } else {
                        iter ++;
                    }
                }

            } catch (const std::exception& e) {
                message()
                    << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                    << e.what();
                show();
                return;
            }
        }
        show(std::make_shared<RegionListView>(std::move(regions)));
    }
};

static RegisterCommand<CommandRegion> _Region {};

} // namespace mypower
