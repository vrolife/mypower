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

#ifndef __scanner_hpp__
#define __scanner_hpp__

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <cassert>
#include <sstream>

#include "comparator.hpp"
#include "process.hpp"
#include "matchvalue.hpp"

#if USE_SIMD
#include <simdjson/arm64/simd.h>
#include <simdjson/dom.h>
#include <simdjson/implementations.h>
#endif

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

namespace mypower {

using namespace std::string_literals;

class MemoryMapper {
    std::shared_ptr<Process> _process;
    VMAddress _begin_addr;
    VMAddress _end_addr;
    size_t _size;
    size_t _step;
    void* _cache { nullptr };
    void* _backup;
    size_t _cache_capacity;
    size_t _page_size;

    ssize_t _cached_size { 0 };
    ssize_t _backup_size { 0 };
    void* _begin { nullptr };
    void* _end { nullptr };

public:
    MemoryMapper(std::shared_ptr<Process>& process, VMAddress begin_addr, VMAddress end_addr, size_t step, size_t cache_capacity = 8 * 1024 * 1024 /* 8M Byte */)
        : _process(process)
        , _begin_addr(begin_addr)
        , _end_addr { end_addr }
        , _step(step)
        , _cache_capacity(cache_capacity)
        , _page_size(sysconf(_SC_PAGESIZE))
    {
        _cache = mmap(nullptr, _cache_capacity + (2 * _page_size), PROT_READ | PROT_WRITE,
            MAP_ANON | MAP_PRIVATE, -1, 0);
        if (_cache == MAP_FAILED) {
            throw std::runtime_error("Out of memory");
        }
        _backup = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(_cache) + _cache_capacity + _page_size);
    }

    ~MemoryMapper()
    {
        if (_cache != nullptr) {
            munmap(_cache, _cache_capacity);
        }
    }

    size_t step() const
    {
        return _step;
    }

    VMAddress address_begin() const { return _begin_addr; }
    VMAddress address_end() const { return _end_addr; }

    void* begin() { return _begin; }
    void* end() { return _end; }

    bool next()
    {
        auto addr = _begin_addr + _cached_size;
        if (addr >= _end_addr) {
            return false;
        }
        void* cache = _backup_size == 0 ? _cache : reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(_cache) + _page_size);
        _cached_size = _process->read(addr, cache, std::min(_cache_capacity, (_end_addr - addr).get()));
        if (_cached_size == -1) {
            throw std::runtime_error("Read memory failed: "s + strerror(errno));
        }

        auto next_backup_size = _cached_size % _step;

        if (_backup_size == 0) {
            _begin = cache;
        } else {
            _begin = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(_cache) + _page_size - _backup_size);
            memcpy(_begin, _backup, _backup_size);
        }

        _end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(_begin) + _cached_size - next_backup_size);
        memcpy(_backup, _end, next_backup_size);
        _backup_size = next_backup_size;

        _begin_addr = addr;
        return true;
    }
};

class Session {
    std::shared_ptr<Process> _process;
    VMRegion::ListType _memory_regions;
    std::vector<std::unique_ptr<MatchValue>> _matchs;
    size_t _cache_size;

public:
    Session(std::shared_ptr<Process>& process, size_t cache_size)
        : _process(process), _cache_size(cache_size)
    {
    }

    ~Session()
    {
    }
    
    bool update_memory_region()
    {
        _memory_regions = VMRegion::snapshot(_process->pid());
        return not _memory_regions.empty();
    }

    const MatchValue& at(size_t index) const {
        return *_matchs.at(index);
    }

    template <typename T>
    void search(T&& scanner, uint32_t mask = kRegionFlagReadWrite)
    {
        for (auto& region : _memory_regions) {
            if ((region._prot & mask) != mask) {
                continue;
            }

            auto begin = region._begin;
            auto end = region._end;
            auto size = region.size();

            MemoryMapper mapper { _process, begin, end, scanner.step(), _cache_size };

            auto callback = [&](std::unique_ptr<MatchValue>&& value) {
                _matchs.emplace_back(std::move(value));
            };

            while (mapper.next()) {
                scanner(mapper.address_begin(), mapper.begin(), mapper.end(), callback);
            }
        }
    }

    template <typename Filter>
    void filter()
    {
        std::vector<struct iovec> remote {};
        remote.reserve(_matchs.size());
        size_t local_size = 0;
        for (auto& match : _matchs) {
            const auto size = match->_size;
            local_size += size;
            remote.emplace_back(iovec { reinterpret_cast<void*>(match->_addr.get()), size });
        }
        std::vector<uint8_t> local_buffer;
        local_buffer.resize(local_size);
        struct iovec local_vec {
            .iov_base = local_buffer.data(),
            .iov_len = local_buffer.size()
        };
        _process->read(&local_vec, 1, remote.data(), remote.size());

        std::vector<std::unique_ptr<MatchValue>> new_matchs;
        new_matchs.reserve(_matchs.size());

        size_t offset = 0;
        for (auto& match : _matchs) {
            auto* ptr = local_buffer.data() + offset;
            offset += match->_size;
            if (match->filter<Filter>(ptr)) {
                memcpy(match->_data._bytes, ptr, match->_size);
                new_matchs.emplace_back(std::move(match));
            }
        }

        _matchs = std::move(new_matchs);
    }

    template <typename Comparator>
    void filter(Comparator&& comparator)
    {
        std::vector<struct iovec> remote {};
        std::vector<struct iovec> local {};

        remote.reserve(_matchs.size());
        local.reserve(_matchs.size());

        for (auto& match : _matchs) {
            remote.emplace_back(iovec { reinterpret_cast<void*>(match->_addr.get()), match->_size });
            local.emplace_back(iovec { match->_data._bytes, match->_size });
        }

        _process->read(local.data(), local.size(), remote.data(), remote.size());

        std::vector<std::unique_ptr<MatchValue>> new_matchs;
        new_matchs.reserve(_matchs.size());

        for (auto& match : _matchs) {
            if (match->filter(comparator)) {
                new_matchs.emplace_back(std::move(match));
            }
        }
        _matchs = std::move(new_matchs);
    }

    size_t size() const
    {
        return _matchs.size();
    }
};

template <typename Comparator>
class ScanComparator {
    typedef typename Comparator::Type Type;
    static_assert(std::is_integral<Type>::value or std::is_floating_point<Type>::value, "Number only");

    Comparator _comparator;
    size_t _step;

public:
    ScanComparator(Comparator&& comparator, size_t step)
        : _comparator { comparator }, _step(step)
    {
        assert(_step > 0);
    }

    constexpr size_t step() const { return _step; }

    template <typename Callback>
    void operator()(VMAddress addr, void* buffer_begin, void* buffer_end, Callback&& callback)
    {
        // Copy to stack
        const auto step = this->step();
        const auto comparator = _comparator;
#if 0
        if (std::is_same<Comparator, ComparatorEqual<uint8_t>>::value and this->step() == sizeof(Type)) {
            // TODO memchr or SIMD
#if SIMDJSON_IS_ARM64
            using namespace simdjson::arm64;
            simd8<uint8_t> v0 { reinterpret_cast<uint8_t*>(buffer) };
            simd8<uint8_t> v1 { reinterpret_cast<uint8_t*>(buffer) + sizeof(v0) };
            uint64_t mask = simd8x64<bool>(v0 == _value, v1 == _value, v0 == _value, v1 == _value).to_bitmask();
            if (UNLIKELY(mask > 0)) { }
#endif
        }
#endif
        if (step == sizeof(Type)) {
            auto begin = reinterpret_cast<uintptr_t>(buffer_begin);
            auto end = reinterpret_cast<uintptr_t>(buffer_end);
            for (uintptr_t iter = begin; iter != end; iter += step) {
                auto value = *reinterpret_cast<Type*>(iter);
                if (UNLIKELY(comparator(value))) {
                    callback(
                        MatchValue::create(
                            addr + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin)),
                            value));
                }
            }

        } else {
            auto begin = reinterpret_cast<uintptr_t>(buffer_begin);
            auto end = reinterpret_cast<uintptr_t>(buffer_end);
            for (uintptr_t iter = begin; iter != end; iter += step) {
                Type value;
                memcpy(&value, reinterpret_cast<void*>(iter), sizeof(Type));
                if (UNLIKELY(comparator(value))) {
                    callback(
                        MatchValue::create(
                            addr + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin)),
                            value));
                }
            }
        }
    }
};

template<typename T, typename Callable>
class ScanExpression {
    typedef T Type;
    Callable _callable;
    size_t _step;
public:
    ScanExpression(Callable&& callable, size_t step)
        : _callable { std::move(callable) }, _step(step)
    {
        assert(_step > 0);
    }

    constexpr size_t step() const { return _step; }

    template <typename Callback>
    void operator()(VMAddress addr_begin, void* buffer_begin, void* buffer_end, Callback&& callback)
    {
        const auto step = this->step();
        auto begin = reinterpret_cast<uintptr_t>(buffer_begin);
        auto end = reinterpret_cast<uintptr_t>(buffer_end);
        for (uintptr_t iter = begin; iter != end; iter += step) {
            Type value;
            memcpy(&value, reinterpret_cast<void*>(iter), sizeof(Type));
            auto address = addr_begin + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin));
            if (UNLIKELY(_callable(0, value, address.get()))) {
                callback(MatchValue::create(address, value));
            }
        }
    }
};

} // namespace mypower

#endif
