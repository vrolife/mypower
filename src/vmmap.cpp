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
#include <filesystem>
#include <fstream>
#include <regex>
#include <streambuf>
#include <string>

#include "vmmap.hpp"

#define REGEX R"(([0-9a-fA-F]+)-([0-9a-fA-F]+)\s+([rwxps-]{4})\s+([0-9a-f]+)\s+([0-9a-fA-F]+):([0-9a-fA-F]+)\s+(\d+)[ \t\r]*(.*)\n)"

using namespace std::string_literals;
namespace fs = std::filesystem;

namespace mypower {
std::vector<VMRegion> snapshot_impl(const std::string& s)
{
    std::regex exp(REGEX, std::regex_constants::ECMAScript | std::regex_constants::icase);

    auto begin = std::sregex_iterator(s.begin(), s.end(), exp);

    auto end = std::sregex_iterator();

    std::vector<VMRegion> regions;

    for (auto iter = begin; iter != end; ++iter) {
        auto& match = *iter;

        VMRegion region;
        region._begin = VMAddress { std::stoul(match[1], 0, 16) };
        region._end = VMAddress { std::stoul(match[2], 0, 16) };
        region._prot = 0;

        const std::string perms = match[3].str();
        if (perms[0] != '-')
            region._prot |= kRegionFlagRead;
        if (perms[1] != '-')
            region._prot |= kRegionFlagWrite;
        if (perms[2] != '-')
            region._prot |= kRegionFlagExec;

        region._shared = perms[3] == 's';

        region._offset = std::stoull(match[4], 0, 16);

        region._major = std::stoull(match[5], 0, 16);
        region._minor = std::stoull(match[6], 0, 16);
        region._inode = std::stoull(match[7], 0, 10);

        auto s = match[8].str();

        if (s.size()) {
            if (s[0] == '/') {
                auto w = s.find(' ');
                if (w == std::string::npos) {
                    region._file = s;
                } else {
                    region._file = s.substr(0, w);
                    region._desc = s.substr(w);
                }
            } else {
                region._desc = s;
            }
        }

        if (not regions.empty() and regions.at(regions.size() -1)._end == region._begin and region._desc == "[anon:.bss]") {
            region._file = regions.rbegin()->_file;
            region._android_bss = true;
        }

        regions.emplace_back(std::move(region));
    }
    return regions;
}

void VMRegion::string(std::ostringstream& oss)
{
    oss << std::hex
        << "0x"
        << static_cast<uintptr_t>(_begin)
        << "-"
        << "0x"
        << static_cast<uintptr_t>(_end)
        << " ";

    size_t align = 6;
    if (_prot & kRegionFlagRead) {
        oss << "R";
        align -= 1;
    }
    if (_prot & kRegionFlagWrite) {
        oss << "W";
        align -= 1;
    }
    if (_prot & kRegionFlagExec) {
        oss << "X";
        align -= 1;
    }

    align -= 1;
    if (_shared) {
        oss << "S";
    } else {
        align -= 1;
    }

    while (align--) {
        oss << " ";
    }

    oss << std::setw(8) << std::hex << _offset;

    oss << " " << _file << " " << _desc;
}

std::vector<VMRegion> VMRegion::snapshot(std::istream& is)
{
    return snapshot_impl(std::string { std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>() });
}

std::vector<VMRegion> VMRegion::snapshot(const fs::path& maps)
{
    std::ifstream ifs(maps);
    if (!ifs.is_open()) {
        return {};
    }
    return snapshot(ifs);
}

std::vector<VMRegion> VMRegion::snapshot(pid_t pid)
{
    fs::path maps = fs::path("/proc") / std::to_string(pid) / "maps";
    return snapshot(maps);
}
} // mypower
