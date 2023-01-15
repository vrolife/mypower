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
#include <iostream>

#include <boost/program_options.hpp>

#include "scanner.hpp"

namespace po = boost::program_options;
using namespace mypower;

int main(int argc, char *argv[])
{
    pid_t target_pid{-1};
    uintptr_t address{0};
    uintptr_t mask{0};
    uintptr_t target_start{0};
    uintptr_t target_end{0};
    size_t max{16};
    size_t step{16};
    size_t cache_size{0};

    try {
        po::options_description desc("Allowed options");
        desc.add_options()("help", "show help message");
        desc.add_options()("pid", po::value<pid_t>()->default_value(getpid()), "target process pid");
        desc.add_options()("target-start", po::value<uintptr_t>(), "target start");
        desc.add_options()("target-end", po::value<uintptr_t>(), "target end");
        desc.add_options()("address", po::value<uintptr_t>(), "address");
        desc.add_options()("mask", po::value<uintptr_t>()->default_value(UINTPTR_MAX), "mask");
        desc.add_options()("max", po::value<size_t>()->default_value(16), "max depth");
        desc.add_options()("step", po::value<size_t>()->default_value(sizeof(uintptr_t)), "step size");
        desc.add_options()("cache-size", po::value<size_t>()->default_value(8 * 1024 * 1024), "cache size");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (vm.count("pid")) {
            target_pid = vm["pid"].as<pid_t>();
        }

        if (vm.count("address")) {
            address = vm["address"].as<uintptr_t>();
        }

        if (vm.count("mask")) {
            mask = vm["mask"].as<uintptr_t>();
        }
        
        if (vm.count("max")) {
            max = vm["max"].as<size_t>();
        }

        if (vm.count("target-start")) {
            target_start = vm["target-start"].as<uintptr_t>();
        }

        if (vm.count("target-end")) {
            target_end = vm["target-end"].as<uintptr_t>();
        }

        if (vm.count("step")) {
            step = vm["step"].as<size_t>();
        }

        if (vm.count("cache-size")) {
            cache_size = vm["cache-size"].as<size_t>();
        }

    } catch (std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    if (target_pid == -1) {
        std::cerr << "error: Invalid PID" << std::endl;
        return 1;
    }

    if (address == 0) {
        std::cerr << "error: Invalid addresss" << std::endl;
        return 1;
    }

    if (target_start == target_end) {
        std::cerr << "error: target size is zero" << std::endl;
        return 1;
    }

    auto process = std::make_shared<Process>(target_pid);

    Session session{process, cache_size};

    session.scan(ScanComparator<ComparatorMask<uintptr_t>> {{address, mask}, step});

    return 0;
}
