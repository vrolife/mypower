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
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <thread>

#include <boost/json.hpp>
#include <boost/program_options.hpp>

#include "mypower.hpp"
#include "raii.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;
namespace fs = std::filesystem;

namespace mypower {

MYPOWER_RAII_SIMPLE_OBJECT(CFile, FILE, fclose);

class ProcessSnapshot : public Process {
    pid_t _pid;

    VMRegion::ListType _regions {};

    std::map<uintptr_t, size_t> _address_index{};

    std::vector<std::vector<uint8_t>> _memory {};

public:
    ProcessSnapshot(pid_t pid, VMRegion::ListType&& regions, std::vector<std::vector<uint8_t>>&& memory)
        : _pid(pid)
        , _regions(regions)
        , _memory(memory)
    {
        for (size_t idx = 0; idx < regions.size(); ++idx) {
            _address_index.emplace(regions.at(idx)._begin.get(), idx);
        }
    }

    pid_t pid() const override { return _pid; }

    ssize_t read(VMAddress address, void* buffer, size_t size) override
    {
        struct iovec local = {
            .iov_base = const_cast<void*>(buffer),
            .iov_len = size
        };
        struct iovec remote {
            .iov_base = reinterpret_cast<void*>(address.get()),
            .iov_len = size
        };
        return read(&local, 1, &remote, 1);
    }

    ssize_t write(VMAddress address, const void* buffer, size_t size) override
    {
        struct iovec local = {
            .iov_base = const_cast<void*>(buffer),
            .iov_len = size
        };
        struct iovec remote {
            .iov_base = reinterpret_cast<void*>(address.get()),
            .iov_len = size
        };
        return write(&local, 1, &remote, 1);
    }

    ssize_t iovec_copy(struct iovec* dst, size_t dst_count, struct iovec* src, size_t src_count)
    {
        if (src_count == 0 or dst_count == 0) {
            return -1;
        }

        ssize_t sz = 0;
        struct iovec* dbegin = dst;
        struct iovec* dend = dst + dst_count;
        struct iovec* sbegin = src;
        struct iovec* send = src + src_count;

        auto* diter = dbegin;
        auto* siter = sbegin;

        while (diter != dend and siter != send) {
            auto src_size = siter->iov_len;
            void* src_ptr = siter->iov_base;
            auto dst_size = diter->iov_len;
            void* dst_ptr = diter->iov_base;

            while (diter != dend and siter != send) {
                auto n = std::min(src_size, dst_size);
                sz += n;
                memcpy(dst_ptr, src_ptr, n);

                if (dst_size == src_size) {
                    diter++;
                    siter++;
                    break;

                } else if (n == dst_size) {
                    diter++;
                    if (diter == dend) {
                        break;
                    }
                    dst_size = diter->iov_len;
                    dst_ptr = diter->iov_base;
                    src_size -= n;
                    src_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(src_ptr) + n);
                    continue;

                } else if (n == src_size) {
                    siter++;
                    if (siter == send) {
                        break;
                    }
                    src_size = siter->iov_len;
                    src_ptr = siter->iov_base;
                    dst_size -= n;
                    dst_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dst_ptr) + n);
                    continue;
                }
            }
        }
        return sz;
    }

    std::vector<struct iovec> relocate_remote(struct iovec* remote, size_t remote_count)
    {
        if (_regions.empty()) {
            return {};
        }

        std::vector<struct iovec> relocated {};
        relocated.reserve(remote_count);

        auto* end = remote + remote_count;
        for (auto* iovec_iter = remote; iovec_iter != end; ++iovec_iter) {

            auto addr = reinterpret_cast<uintptr_t>(iovec_iter->iov_base);
            auto addr_end = addr + iovec_iter->iov_len;

            auto pair = _address_index.lower_bound(addr);

            assert(not _address_index.empty());

            ssize_t idx = -1;

            if (pair == _address_index.end()) {
                idx = _regions.size() - 1;
            } else {
                idx = pair->second;
            }

            for(; idx >= 0; --idx) 
            {
                auto& region = _regions.at(idx);
                if (addr >= region._begin.get() and addr_end <= region._end.get()) {
                    break;
                }

                if (region._end.get() < addr) {
                    return {};
                }
            }

            if (idx < 0) {
                return {};
            }

            auto& region = _regions.at(idx);

            if (region._prot == 0 or _memory.at(idx).empty()) {
                return {};
            }

            assert(addr >= region._begin.get() and addr_end <= region._end.get());

            relocated.push_back({ reinterpret_cast<void*>(_memory.at(idx).data() + (addr - region._begin.get())), iovec_iter->iov_len });
            assert(relocated.rbegin()->iov_base != nullptr);
        }
        return relocated;
    }

    ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override
    {
        auto relocated = relocate_remote(remote, remote_count);
        return iovec_copy(local, local_count, relocated.data(), relocated.size());
    }

    ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override
    {
        auto relocated = relocate_remote(remote, remote_count);
        return iovec_copy(remote, remote_count, relocated.data(), relocated.size());
    }

    bool suspend(bool same_user = false) override
    {
        return false;
    }

    bool resume(bool same_user = false) override
    {
        return false;
    }

    ProcessState get_process_state() override
    {
        return Stopped;
    }

    VMRegion::ListType get_memory_regions() override
    {
        return _regions;
    }
};

class CommandSnapshot : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandSnapshot(Application& app)
        : Command(app, "snapshot")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("load", po::bool_switch()->default_value(false), "load snapshot");
        _options.add_options()("prefix", po::value<std::string>(), "prefix");
        _posiginal.add("prefix", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "snapshot";
    }

    void show_short_help() override
    {
        message() << "snapshot\t\tSave process's memory to file";
    }

    void save_process(std::string prefix)
    {
        if (prefix.empty()) {
            prefix = "dump";
        }

        if (fs::exists(prefix + ".json")) {
            size_t idx = 0;
            std::string next_file_name {};

            do {
                idx += 1;
                next_file_name = prefix;
                next_file_name.append(std::to_string(idx));
                next_file_name.append(".json");
            } while (fs::exists(next_file_name));

            prefix.append(std::to_string(idx));
        }

        AutoSuspendResume suspend { _app._process };

        boost::json::object jobject {};
        boost::json::array jregions {};

        size_t memory_size {};
        size_t buffer_size { 4 * 1024 * 1024 };

        std::vector<uint8_t> in_buffer {};
        auto regions = _app._process->get_memory_regions();

        std::string memory_file_name = prefix + ".memory";
        CFile memory_file { fopen(memory_file_name.c_str(), "wb") };
        if (memory_file == nullptr) {
            message()
                << attributes::SetColor(attributes::ColorError)
                << "Unable to open file: " << memory_file_name << strerror(errno);
            show();
            return;
        }

        in_buffer.resize(buffer_size);

        size_t saved_offset = 0;

        for (auto& region : regions) {
            boost::json::object item {};
            size_t saved_size { 0 };

            if (region._prot) {
                memory_size += region.size();

                auto begin = region._begin;
                auto end = region._end;

                while (begin < end) {
                    auto min_size = std::min(in_buffer.size(), end.get() - begin.get());
                    auto read_size = _app._process->read(begin, in_buffer.data(), min_size);

                    if (read_size == -1) {
                        break;
                    }

                    if (fwrite(in_buffer.data(), read_size, 1, memory_file) != 1) {
                        message() 
                            << attributes::SetColor(attributes::ColorError) 
                            << "Out of disk space: " << std::hex
                            << region._begin.get() << "-" << region._end.get()
                            << " " << region._file;
                        show();
                        return;
                    }

                    saved_size += read_size;

                    begin = VMAddress{begin.get() + read_size};
                }

                if (begin != end) {
                    region._prot = 0;
                    message() 
                        << attributes::SetColor(attributes::ColorError) 
                        << "Save region failed: " << std::hex
                        << region._begin.get() << "-" << region._end.get()
                        << " " << region._file;
                    show();
                }
            }

            item.insert_or_assign("begin", region._begin.get());
            item.insert_or_assign("end", region._end.get());
            item.insert_or_assign("prot", region._prot);
            item.insert_or_assign("shared", region._shared);
            item.insert_or_assign("file", region._file);
            item.insert_or_assign("desc", region._desc);
            item.insert_or_assign("offset", region._offset);
            item.insert_or_assign("major", region._major);
            item.insert_or_assign("minor", region._minor);
            item.insert_or_assign("inode", region._inode);
            item.insert_or_assign("deleted", region._deleted);
            item.insert_or_assign("saved_size", saved_size);
            item.insert_or_assign("saved_offset", saved_offset);
            jregions.emplace_back(std::move(item));

            saved_offset += saved_size;
        }

        jobject.insert_or_assign("regions", jregions);
        jobject.insert_or_assign("memory_size", memory_size);
        jobject.insert_or_assign("memory_file", prefix + ".memory");
        jobject.insert_or_assign("pid", _app._process->pid());

        std::ofstream info_file { prefix + ".json" };
        info_file << jobject;

        message() << "memory size: " << memory_size;
        message() << "writen size: " << saved_offset;
        show();
    }

    void load_process(const std::string& prefix)
    {
        pid_t pid { -1 };
        VMRegion::ListType _regions {};
        std::vector<std::vector<uint8_t>> _memory {};

        if (not fs::exists(prefix)) {
            message() << attributes::SetColor(attributes::ColorError) << "File does not exists: " << prefix;
            show();
            return;
        }

        std::string info_buffer {};
        info_buffer.resize(fs::file_size(prefix));

        std::ifstream infofile(prefix, std::ios::binary | std::ios::in);

        infofile.read(info_buffer.data(), info_buffer.size());

        auto object = boost::json::parse(info_buffer);

        pid = object.at("pid").as_int64();
        size_t memory_size = object.at("memory_size").as_int64();
        auto memory_file_name = object.at("memory_file").as_string();

        auto jregions = object.at("regions").as_array();
        _regions.reserve(jregions.size());
        _memory.resize(jregions.size());

        CFile memfile { fopen(memory_file_name.c_str(), "rb") };
        if (memfile == nullptr) {
            message()
                << attributes::SetColor(attributes::ColorError)
                << "Unable to open file: " << memory_file_name << strerror(errno);
            show();
            return;
        }

        size_t idx = 0;
        for (auto& item : jregions) {
            VMRegion region {};
            region._begin = VMAddress { boost::json::value_to<uintptr_t>(item.at("begin")) };
            region._end = VMAddress { boost::json::value_to<uintptr_t>(item.at("end")) };
            region._prot = (uint32_t)item.at("prot").as_int64();
            region._shared = item.at("shared").as_bool();
            region._file = item.at("file").as_string();
            region._desc = item.at("desc").as_string();
            region._offset = item.at("offset").as_int64();
            region._major = item.at("major").as_int64();
            region._minor = item.at("minor").as_int64();
            region._inode = item.at("inode").as_int64();
            region._deleted = item.at("deleted").as_bool();

            auto saved_size = item.at("saved_size").as_int64();
            auto saved_offset = item.at("saved_offset").as_int64();

            if (saved_size) {
                auto& mem = _memory.at(idx);
                mem.resize(region.size());
                fseek(memfile, saved_offset, SEEK_SET);
                if (fread(mem.data(), saved_size, 1, memfile) != 1) {
                    mem.clear();
                    mem.shrink_to_fit();
                    region._prot = 0;
                }
            }
            _regions.push_back(region);
            idx += 1;
        }

        message() << "Attach process " << pid;
        show();

        _app._process = std::make_shared<ProcessSnapshot>(pid, std::move(_regions), std::move(_memory));
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        std::string prefix {};
        bool load { false };

        try {
            if (opts.count("prefix")) {
                prefix = opts["prefix"].as<std::string>();
            }

            load = opts["load"].as<bool>();

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

        try {
            if (load) {
                load_process(prefix);
            } else {
                if (not _app._process) {
                    message()
                        << SetColor(ColorError)
                        << "Error:"
                        << ResetStyle()
                        << " Invalid target process, attach to target using the 'attach' command";
                    show();
                    return;
                }
                save_process(prefix);
            }
        } catch (const std::exception& e) {
            message()
                << SetColor(ColorError) << "Error:" << ResetStyle() << " "
                << e.what();
            show();
            return;
        }
    }
};

static RegisterCommand<CommandSnapshot> _Test {};

} // namespace mypower
