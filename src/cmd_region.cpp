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

std::string get_human_readable_size(size_t size) {
    static const char* units[] = {
        "B",
        "KB",
        "MB",
        "GB",
        "TB",
        "PB",
        "EB",
        "ZB",
        "YB",
        "RB",
        "QB",
    };

    constexpr size_t pow = 1024;
    size_t unit = 0;

    std::vector<std::string> tmp{};

    while(size) {
        std::ostringstream oss;
        oss << (size % pow) << "\t" << units[unit];
        tmp.emplace_back(oss.str());

        size = size / pow;
        ++unit;
    }

    std::ostringstream oss;
    for (auto iter = tmp.rbegin(); iter != tmp.rend(); ++iter) {
        oss << *iter << "\t";
    }
    return oss.str();
};

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
    
    bool tui_key(size_t index, int key) override
    {
        switch (key) {
        case 's':
            std::sort(_regions.begin(), _regions.end(), [](auto& a, auto& b){
                return a.size() < b.size();
            });
            this->tui_notify_changed();
            return true;
        case 'S':
            std::sort(_regions.begin(), _regions.end(), [](auto& a, auto& b){
                return a.size() > b.size();
            });
            this->tui_notify_changed();
            return true;
        }
        return false;
    }
};

class CommandRegion : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandRegion(Application& app)
        : Command(app, "region")
    {
        _options.add_options()("info", po::bool_switch()->default_value(false), "show memory information");
        _options.add_options()("regex,r", po::bool_switch()->default_value(false), "regex filter");
        _options.add_options()("filter", po::value<std::string>(), "filter");
        _posiginal.add("filter", 1);
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

        if (opts["info"].as<bool>())
        {
            size_t RO{0};
            size_t RW{0};
            size_t WX{0};
            size_t RX{0};
            size_t SHARED{0};
            size_t FILE{0};
            size_t DEV{0};
            size_t NONE{0};

            for(auto& region : regions) {
                if (region._prot == 0) {
                    NONE += region.size();
                }
                if (region._prot == kRegionFlagRead) {
                    RO += region.size();
                }
                if (region._prot == kRegionFlagReadWrite) {
                    RW += region.size();
                }
                if (region._prot == (kRegionFlagWrite | kRegionFlagExec)) {
                    WX += region.size();
                }
                if (region._prot == (kRegionFlagRead | kRegionFlagExec)) {
                    RX += region.size();
                }
                if (region._shared) {
                    SHARED += region.size();
                }
                if (not region._file.empty()) {
                    FILE += region.size();
                }
                if (region._file.find("/dev/") == 0) {
                    DEV += region.size();
                }
            }

#define OUTPUT(n) message() << #n << "\t" << get_human_readable_size(n);
            OUTPUT(RO);
            OUTPUT(RW);
            OUTPUT(WX);
            OUTPUT(RX);
            OUTPUT(SHARED);
            OUTPUT(FILE);
            OUTPUT(DEV);
            OUTPUT(NONE);
            return;
        }

        if (not filter.empty()) {
            try {
                std::regex regexp { filter };

                for(auto iter = regions.begin(); iter != regions.end();) {
                    if (not std::regex_match(iter->_file, regexp) and not std::regex_match(iter->_desc, regexp)) {
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
