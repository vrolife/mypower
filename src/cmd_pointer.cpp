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

#include "mathexpr.hpp"
#include "mypower.hpp"
#include "scanner.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

struct GotIt : public std::logic_error {
    using std::logic_error::logic_error;
};

struct PointerConfig {
    uintptr_t begin;
    uintptr_t end;
    uintptr_t mask;
    size_t step;
    size_t depth_max;
    size_t offset_max;
    size_t result_max;
    bool find_all;
};

struct PointerInfo {
    uintptr_t _pointer;
    uintptr_t _value{0};
    uintptr_t _offset{0};
    size_t _depth{0};
    PointerInfo* _prev{nullptr};
};

class CommandPointer : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandPointer(Application& app)
        : Command(app, "pointer")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("begin", po::value<std::string>(), "target region start");
        _options.add_options()("end", po::value<std::string>(), "target region end");
        _options.add_options()("pointer", po::value<std::string>(), "pointer");
        _options.add_options()("mask", po::value<std::string>(), "mask");
        _options.add_options()("depth-max", po::value<size_t>()->default_value(5), "depth max");
        _options.add_options()("offset-max", po::value<size_t>()->default_value(1024), "offset max");
        _options.add_options()("result-max", po::value<size_t>()->default_value(1024), "result max");
        _options.add_options()("step", po::value<size_t>()->default_value(sizeof(uintptr_t)), "step size");
        _options.add_options()("all", po::bool_switch()->default_value(false), "find all");
        _posiginal.add("begin", 1);
        _posiginal.add("end", 1);
        _posiginal.add("pointer", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "ptr" or command == "pointer";
    }

    static const std::string_view pad(size_t depth) {
        if (depth < 10) {
            return { "                    ", depth * 2 };
        }
        return { "    <Level >= 10>    ", 21 };
    }

    void print_path(PointerInfo& ptr) {
        auto depth = ptr._depth;
        auto* p = &ptr;
        while (p != nullptr) {
            if (p->_prev) {
                message() 
                    << pad(depth - p->_depth)
                    << (void*)p->_pointer
                    << " Value: " << (void*)p->_value
                    << " Offset: " << p->_offset;
            } else {
                message() 
                    << pad(depth - p->_depth)
                    << attributes::SetColor(attributes::ColorInfo)
                    << (void*)p->_pointer;
            }
            p = p->_prev;
        }
    }

    void find_ref(PointerInfo& ptr, PointerConfig& config) 
    {
        if (ptr._depth >= config.depth_max) {
            return;
        }

        Session session{_app._process, 8 * 1024 * 1024};

        session.update_memory_region();
        
        session.scan(ScanComparator<ComparatorMask<uintptr_t>>{ { ptr._pointer, config.mask }, config.step }, kRegionFlagReadWrite);

        if (session.U64_size() > config.result_max) {
            message() << pad(ptr._depth) << "Too many result " << session.U64_size();
            return;
        }

        std::sort(session.U64_begin(), session.U64_end(), [=](const MatchU64& a, const MatchU64& b){
            auto x = ptr._pointer - a._value;
            auto y = ptr._pointer - b._value;
            return x < y;
        });
        
        for (auto iter = session.U64_begin(); iter != session.U64_end(); ++iter) {
            if (iter->_value > ptr._pointer) {
                continue;
            }

            if (iter->_addr.get() >= config.begin and iter->_addr.get() < config.end) {
                // message() 
                //     << pad(ptr._depth) << attributes::SetColor(attributes::ColorInfo) 
                //     << "Got it: " << attributes::ResetStyle() << (void*)iter->_addr.get() 
                //     << " Value: " << (void*)iter->_value 
                //     << " Offset: " << (ptr._pointer - iter->_value);

                PointerInfo next{};
                next._pointer = iter->_addr.get();
                next._value = iter->_value;
                next._depth = ptr._depth + 1;
                next._offset = ptr._pointer - iter->_value;
                next._prev = &ptr;
                print_path(next);

                if (not config.find_all) {
                    throw GotIt("got it");
                }
                continue;
            }

            if ((ptr._pointer - iter->_value) < config.offset_max) {
                // message() 
                //     << pad(ptr._depth) 
                //     << "Next: " << (void*)iter->_addr.get() 
                //     << " Value: " << (void*)iter->_value 
                //     << " Offset: " << (ptr._pointer - iter->_value);
                PointerInfo next{};
                next._pointer = iter->_addr.get();
                next._value = iter->_value;
                next._depth = ptr._depth + 1;
                next._offset = ptr._pointer - iter->_value;
                next._prev = &ptr;
                find_ref(next, config);
            }
        }
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();
        uintptr_t pointer { 0 };
        uintptr_t mask { 0 };
        uintptr_t begin { 0 };
        uintptr_t end { 0 };
        size_t depth_max { 0 };
        size_t offset_max { 0 };
        size_t result_max { 0 };
        size_t step { sizeof(void*) };

        if (opts.count("help")) {
            message() << _options;
            show();
            return;
        }

        try {
            if (opts.count("begin")) {
                begin = mathexpr::parse_address_or_throw(opts["begin"].as<std::string>());
            }
            if (opts.count("end")) {
                end = mathexpr::parse_address_or_throw(opts["end"].as<std::string>());
            }
            if (opts.count("pointer")) {
                pointer = mathexpr::parse_address_or_throw(opts["pointer"].as<std::string>());
            }
            if (opts.count("mask")) {
                mask = mathexpr::parse_address_or_throw(opts["mask"].as<std::string>());
            } else {
                mask = UINTPTR_MAX - 0x3FF;
            }
            depth_max = opts["depth-max"].as<size_t>();
            offset_max = opts["offset-max"].as<size_t>();
            result_max = opts["result-max"].as<size_t>();
            step = opts["step"].as<size_t>();

        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        try {
            PointerConfig config;
            config.begin = begin;
            config.end = end;
            config.mask = mask;
            config.step = step;
            config.depth_max = depth_max;
            config.offset_max = offset_max;
            config.result_max = result_max;
            config.find_all = opts["all"].as<bool>();

            PointerInfo ptr_info{};
            ptr_info._pointer = pointer;

            auto t0 = std::chrono::system_clock::now();

            message() 
                << "From: " << (void*)begin << "-" << (void*)end 
                << " To: " << EnableStyle(AttrUnderline) << SetColor(ColorInfo) << (void*)pointer;
            find_ref(ptr_info, config);

            auto d = std::chrono::system_clock::now() - t0;
            message() << "Time: " << std::chrono::duration_cast<std::chrono::seconds>(d).count() << "s";

        } catch (const GotIt&) {
            // do nothing

        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }
    }
};

static RegisterCommand<CommandPointer> _Pointer {};

} // namespace mypower
