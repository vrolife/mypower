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

class CommandU3D : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandU3D(Application& app)
        : Command(app, "region")
    {
        // _options.add_options()("regex,r", po::bool_switch()->default_value(false), "regex filter");
        // _options.add_options()("filter", po::value<std::string>(), "filter");
        // _posiginal.add("filter", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "u3d";
    }

    void show_short_help() override {
        message() << "u3d\t\t\tFind unity3d object";
    }

    bool read_u3d_class_name(VMAddress vmaddr, std::string& buffer, size_t max=64)
    {
        uintptr_t class_ptr { 0 };
        uintptr_t name_ptr { 0 };

        if (_app._process->read(vmaddr, &class_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return false;
        }

        if (_app._process->read(VMAddress { class_ptr + 2 * sizeof(uintptr_t) }, &name_ptr, sizeof(uintptr_t)) != sizeof(uintptr_t)) {
            return false;
        }

        buffer.resize(max);
        if (_app._process->read(VMAddress { name_ptr }, buffer.data(), buffer.size()) != buffer.size()) {
            return false;
        }

        auto idx = buffer.find((char)0);
        if (idx == std::string::npos or idx == 0) {
            return false;
        }
        buffer.resize(idx);
        return true;
    }

    void find_u3d_object(VMAddress begin, VMAddress end, std::vector<std::pair<uintptr_t, std::string>>& results) {
        std::vector<uintptr_t> memory{};
        memory.resize((end.get() - begin.get()) / sizeof(uintptr_t));
        if (_app._process->read(begin, memory.data(), end.get() - begin.get()) == -1) {
            message() << "U3D: Failed to read memory";
        }

        #pragma omp parallel
        {
            std::string buffer{};
            buffer.reserve(64);

            std::vector<std::pair<uintptr_t, std::string>> per_thread_results{};

            #pragma omp for nowait schedule(static)
            for (auto addr : memory) {
                if (read_u3d_class_name(VMAddress{addr}, buffer) and std::isalpha(buffer.at(0))
                    and memcmp(buffer.data(), "Project8Game", 12) == 0
                ) {
                    per_thread_results.emplace_back(addr, buffer);
                }
            }

            #pragma omp critical
            results.insert(results.end(), per_thread_results.begin(), per_thread_results.end());
        }
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        std::vector<std::pair<uintptr_t, std::string>> results{};

        for (auto& region : _app._process->get_memory_regions()) {
            if ((region._prot & kRegionFlagWrite) == 0) {
                continue;
            }
            find_u3d_object(region._begin, region._end, results);
        }

        for (auto pair : results) {
            message() << "U3D Object: 0x" << std::hex << pair.first << " " << pair.second;
        }
    }
};

static RegisterCommand<CommandU3D> _U3D {};

} // namespace mypower
