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
#include <random>
#include <thread>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <condition_variable>

#include <boost/program_options.hpp>
#include <boost/json.hpp>

#include <zstd.h>

#include "raii.hpp"
#include "mypower.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;
namespace fs = std::filesystem;

namespace mypower {

MYPOWER_RAII_SIMPLE_OBJECT(ZstdCompressStream, ::ZSTD_CStream, ZSTD_freeCStream);
MYPOWER_RAII_SIMPLE_OBJECT(CFile, FILE, fclose);

class ProcessSnapshot : public Process {
    pid_t _pid;

    VMRegion::ListType _regions{};

    std::vector<std::vector<uint8_t>> _memory{};

public:
    ProcessSnapshot(pid_t pid, VMRegion::ListType&& regions, std::vector<std::vector<uint8_t>>&& memory)
        : _pid(pid), _regions(regions), _memory(memory)
    {
    }

    pid_t pid() const override { return _pid; }

    ssize_t read(VMAddress address, void* buffer, size_t size) override {
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

    ssize_t write(VMAddress address, const void* buffer, size_t size) override {
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

    ssize_t iovec_copy(struct iovec* dst, size_t dst_count, struct iovec* src, size_t src_count) {
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
        
        while (diter != dend and siter != send) 
        {
            auto src_size = siter->iov_len;
            void* src_ptr = siter->iov_base;
            auto dst_size = diter->iov_len;
            void* dst_ptr = diter->iov_base;

            while (diter != dend and siter != send) {
                auto n = std::min(src_size, dst_size);
                sz += n;
                memcpy(dst_ptr, src_ptr, n);

                if (dst_size == src_size) {
                    diter ++;
                    siter ++;
                    break;

                } else if (n == dst_size) {
                    diter ++;
                    if (diter == dend) {
                        break;
                    }
                    dst_size = diter->iov_len;
                    dst_ptr = diter->iov_base;
                    src_size -= n;
                    src_ptr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(src_ptr) + n);
                    continue;

                } else if (n == src_size) {
                    siter ++;
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

    std::vector<struct iovec> relocate_remote(struct iovec* remote, size_t remote_count) {
        std::vector<struct iovec> relocated{};
        relocated.reserve(remote_count);

        auto* end = remote + remote_count;
        for (auto* iter = remote; iter != end; ++iter) {
            size_t idx = 0;

            for (auto& region : _regions) {
                auto addr = reinterpret_cast<uintptr_t>(iter->iov_base);
                if (addr >= region._begin.get() and addr < region._end.get()) {
                    relocated.push_back({
                        reinterpret_cast<void*>(_memory.at(idx).data() + (addr - region._begin.get())),
                        iter->iov_len
                    });
                    break;
                }
                idx += 1;
            }

            // not found
            if (idx == _memory.size()) {
                return {};
            }
        }
        return relocated;
    }

    ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override {
        auto relocated = relocate_remote(remote, remote_count);
        return iovec_copy(local, local_count, relocated.data(), relocated.size());
    }

    ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count) override {
        auto relocated = relocate_remote(remote, remote_count);
        return iovec_copy(remote, remote_count, relocated.data(), relocated.size());
    }

    bool suspend(bool same_user = false) override {
        return false;
    }

    bool resume(bool same_user = false) override {
        return false;
    }

    ProcessState get_process_state() override {
        return Stopped;
    }

    VMRegion::ListType get_memory_regions() override {
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
        _options.add_options()("compress", po::bool_switch()->default_value(true), "compress");
        _options.add_options()("compress-level", po::value<int>()->default_value(6), "compress level");
        _options.add_options()("filename", po::value<std::string>(), "filename");
        _posiginal.add("filename", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "snapshot";
    }

    void show_short_help() override {
        message() << "snapshot\t\tSave process's memory to file";
    }

    void save_process(std::string filename, bool compressed, int compressed_level) 
    {
        if (filename.empty()) {
            filename = "dump";
        }

        if (fs::exists(filename + ".json")) {
            size_t idx = 0;
            std::string next_file_name{};

            do {
                idx += 1;
                next_file_name = filename;
                next_file_name.append(std::to_string(idx));
                next_file_name.append(".json");
            } while(fs::exists(next_file_name));

            filename.append(std::to_string(idx));
        }

        AutoSuspendResume suspend{_app._process};

        boost::json::object jobject{};
        boost::json::array jregions{};

        size_t memory_size{};
        size_t buffer_size{0};
        std::vector<uint8_t> in_buffer{};
        std::vector<uint8_t> out_buffer{};
        auto regions = _app._process->get_memory_regions();

        std::string memory_file_name = filename + ".memory";
        CFile memory_file{fopen(memory_file_name.c_str(), "wb")};
        if (memory_file == nullptr) {
            message() 
                << attributes::SetColor(attributes::ColorError) 
                << "Unable to open file: " << memory_file_name << strerror(errno);
            show();
            return;
        }

        ZstdCompressStream stream{ZSTD_createCStream()};
        ZSTD_initCStream(stream, 6);

        for (auto& region : regions) {
            if (region._prot and region.size() > buffer_size) {
                buffer_size = region.size();
                memory_size += region.size();
            }
        }

        in_buffer.resize(buffer_size);
        out_buffer.resize(buffer_size);

        size_t saved_offset = 0;

        for (auto& region : regions) 
        {
            boost::json::object item{};
            size_t saved_size{0};
            
            if (region._prot) {
                if (_app._process->read(region._begin, in_buffer.data(), region.size()) == region.size()) {
                    if (compressed) {
                        saved_size = ZSTD_compress(out_buffer.data(), out_buffer.size(), in_buffer.data(), region.size(), 
                            compressed_level);
                        fwrite(out_buffer.data(), saved_size, 1, memory_file);
                    } else {
                        saved_size = region.size();
                        fwrite(in_buffer.data(), saved_size, 1, memory_file);
                    }
                } else {
                    region._prot = 0;
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
        jobject.insert_or_assign("memory_file", filename + ".memory");
        jobject.insert_or_assign("compressed", compressed);
        jobject.insert_or_assign("pid", _app._process->pid());
        
        std::ofstream info_file{filename + ".json"};
        info_file << jobject;

        message() << "memory size: " << memory_size;
        message() << "writen size: " << saved_offset;
        show();
    }

    void load_process(const std::string& filename) {
        pid_t pid{-1};
        VMRegion::ListType _regions{};
        std::vector<std::vector<uint8_t>> _memory{};

        if (not fs::exists(filename)) {
            message() << attributes::SetColor(attributes::ColorError) << "File does not exists: " << filename;
            show();
            return;
        }

        std::string info_buffer{};
        info_buffer.resize(fs::file_size(filename));

        std::ifstream infofile(filename, std::ios::binary|std::ios::in);

        infofile.read(info_buffer.data(), info_buffer.size());

        auto object = boost::json::parse(info_buffer);

        pid = object.at("pid").as_int64();
        bool compressed = object.at("compressed").as_bool();
        size_t memory_size = object.at("memory_size").as_int64();
        auto memory_file_name = object.at("memory_file").as_string();
        
        auto jregions = object.at("regions").as_array();
        _regions.reserve(jregions.size());
        _memory.resize(jregions.size());

        std::vector<uint8_t> mem_buffer{};

        CFile memfile{fopen(memory_file_name.c_str(), "rb")};
        if (memfile == nullptr) {
            message() 
                << attributes::SetColor(attributes::ColorError) 
                << "Unable to open file: " << memory_file_name << strerror(errno);
            show();
            return;
        }

        size_t idx = 0;
        for (auto& item : jregions) {
            VMRegion region{};
            region._begin = VMAddress{boost::json::value_to<uintptr_t>(item.at("begin"))};
            region._end = VMAddress{boost::json::value_to<uintptr_t>(item.at("end"))};
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

                if (compressed) {
                    if (mem_buffer.size() < saved_size) {
                        mem_buffer.resize(saved_size);
                    }
                    fread(mem_buffer.data(), saved_size, 1, memfile);
                    auto ret = ZSTD_decompress(mem.data(), mem.size(), mem_buffer.data(), saved_size);
                    if (ZSTD_isError(ret)) {
                        region._prot = 0;
                        message() 
                            << attributes::SetColor(attributes::ColorError) 
                            << "Decompress region failed: " << std::hex
                            << region._begin.get() << "-" << region._end.get()
                            << " " << region._file;
                        show();
                    }
                } else {
                    fread(mem.data(), saved_size, 1, memfile);
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

        std::string filename{};
        bool load{false};

        try {
            if (opts.count("filename")) {
                filename = opts["filename"].as<std::string>();
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
            message() << "Usage: " << command << " [options] filename\n"
                      << _options;
            show();
            return;
        }

        try {
            if (load) {
                load_process(filename);
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
                save_process(filename, opts["compress"].as<bool>(), opts["compress-level"].as<int>());
            }
        } catch(const std::exception& e) {
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
