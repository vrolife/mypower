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
        _options.add_options()("prefix", po::value<std::string>()->default_value({}), "prefix");
        _options.add_options()("begin", po::value<std::string>(), "begin");
        _options.add_options()("end", po::value<std::string>(), "end");
        _posiginal.add("prefix", 1);
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

    void find_u3d_object(VMAddress begin, VMAddress end, std::vector<std::pair<uintptr_t, std::string>>& results, const std::string& prefix) {
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
                    and (prefix.empty() or memcmp(buffer.data(), prefix.c_str(), prefix.size()) == 0)
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
        PROGRAM_OPTIONS();

        std::string prefix {};

        try {
            if (opts.count("prefix")) {
                prefix = opts["prefix"].as<std::string>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }
        if (opts.count("help")) {
            message() << "Usage: " << command << " [options] prefix\n"
                      << _options;
            show();
            return;
        }

        std::vector<std::pair<uintptr_t, std::string>> results{};

        if (opts.count("begin") and opts.count("end")) {
            auto begin_ast = mathexpr::parse(opts["begin"].as<std::string>());
            auto begin_number_ast = dynamic_cast<mathexpr::ASTNumber*>(begin_ast.get());
            if (begin_ast == nullptr) {
                message()
                    << SetColor(ColorError)
                    << "Error:"
                    << ResetStyle()
                    << " Unsupported address expression";
                show();
                return;
            }
            auto end_ast = mathexpr::parse(opts["end"].as<std::string>());
            auto end_number_ast = dynamic_cast<mathexpr::ASTNumber*>(end_ast.get());
            if (end_ast == nullptr) {
                message()
                    << SetColor(ColorError)
                    << "Error:"
                    << ResetStyle()
                    << " Unsupported address expression";
                show();
                return;
            }
            find_u3d_object(VMAddress{begin_number_ast->_value}, VMAddress{end_number_ast->_value}, results, prefix);
        } else {
            for (auto& region : _app._process->get_memory_regions()) {
                if ((region._prot & kRegionFlagWrite) == 0) {
                    continue;
                }
                find_u3d_object(region._begin, region._end, results, prefix);
            }
        }

        for (auto pair : results) {
            message() << "U3D Object: 0x" << std::hex << pair.first << " " << pair.second;
        }
    }
};

static RegisterCommand<CommandU3D> _U3D {};

} // namespace mypower
