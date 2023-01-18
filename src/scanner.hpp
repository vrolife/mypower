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
#include "matchvalue.hpp"
#include "process.hpp"

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
        auto read_size = std::min(_cache_capacity, (_end_addr - addr).get());
        _cached_size = _process->read(addr, cache, read_size);
        if (_cached_size == -1) {
            std::ostringstream oss {};
            oss << "Read memory failed: "s + strerror(errno) << ". "
                << std::hex << "0x" << addr.get() << " (0x" << read_size << ")";
            throw std::runtime_error(oss.str());
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
    size_t _cache_size;

#define __MATCHES(t) std::vector<Match##t> _matches_##t;
    MATCH_TYPES(__MATCHES);
#undef __MATCHES

public:
    Session(std::shared_ptr<Process>& process, size_t cache_size)
        : _process(process)
        , _cache_size(cache_size)
    {
    }

    ~Session()
    {
    }

    void update_memory_region()
    {
        _memory_regions = _process->get_memory_regions();
    }

    template <typename T>
    void update_memory_region(T&& regions)
    {
        _memory_regions = std::forward<T>(regions);
    }

    template <typename T>
    void find_region(VMAddress addr, T&& cb)
    {
        for (auto& region : _memory_regions) {
            if (addr >= region._begin and addr < region._end) {
                cb(region);
                break;
            }
        }
    }

    void reset()
    {
        _memory_regions.clear();

#define __RESET(t) \
    _matches_##t.clear();
        MATCH_TYPES(__RESET);
#undef __RESET
    }

    template <typename T>
    const auto& get() const
    {
#define __GET(t)                                     \
    if constexpr (std::is_same<T, type##t>::value) { \
        return _matches_##t;                         \
    }

        MATCH_TYPES(__GET);
#undef __GET
    }

    size_t size() const
    {
        size_t sz = 0;
#define __SIZE(t) \
    sz += _matches_##t.size();
        MATCH_TYPES(__SIZE);
#undef __SIZE
        return sz;
    }

    std::unique_ptr<AccessMatch> access(size_t index) const
    {
        size_t offset = 0;
#define __ACCESS(t)                                                   \
    if (index >= offset and index < (offset + _matches_##t.size())) { \
        return access_match(_matches_##t.at(index - offset));         \
    }                                                                 \
    offset += _matches_##t.size();

        MATCH_TYPES(__ACCESS);
#undef __ACCESS
        std::ostringstream oss{};
        oss << "index out of range: index: " << index << " size: " << size();
        throw std::out_of_range(oss.str());
    }

#define __SIZE(t)                   \
    size_t t##_size() const         \
    {                               \
        return _matches_##t.size(); \
    }

    MATCH_TYPES(__SIZE);
#undef __SIZE

#define __AT(t)                        \
    auto t##_at(size_t index) const    \
    {                                  \
        return _matches_##t.at(index); \
    }

    MATCH_TYPES(__AT);
#undef __AT

    template <typename T>
    void add_match(T&& match)
    {
#define __ADD_MATCH(t)                                \
    if constexpr (std::is_same<T, Match##t>::value) { \
        _matches_##t.emplace_back(std::move(match));  \
    }
        MATCH_TYPES(__ADD_MATCH);
#undef __ADD_MATCH
    }

    template <typename T>
    void scan(T&& scanner, uint32_t mask = kRegionFlagReadWrite)
    {
        for (auto& region : _memory_regions) {
            if ((region._prot & mask) != mask) {
                continue;
            }

            auto begin = region._begin;
            auto end = region._end;
            auto size = region.size();

            MemoryMapper mapper { _process, begin, end, scanner.step(), _cache_size };

            auto callback = [&](typename T::MatchType&& value) {
                add_match(std::move(value));
            };

            try {
                while (mapper.next()) {
                    scanner(mapper.address_begin(), mapper.begin(), mapper.end(), callback);
                }
            } catch (...) {
                continue;
            }
        }
    }

    template <typename Filter, typename M>
    void filter(M& matches, const Filter& filter = {})
    {
        typedef typename M::value_type MatchType;
        typedef typename MatchType::type ValueType;

        if (matches.empty()) {
            return;
        }

        std::vector<struct iovec> remote {};
        remote.reserve(matches.size());
        size_t local_size = 0;
        for (auto& match : matches) {
            const auto size = sizeof(ValueType);
            local_size += size;
            remote.emplace_back(iovec { reinterpret_cast<void*>(match._addr.get()), size });
        }
        std::vector<uint8_t> local_buffer;
        local_buffer.resize(local_size);
        struct iovec local_vec {
            .iov_base = local_buffer.data(),
            .iov_len = local_buffer.size()
        };
        _process->read(&local_vec, 1, remote.data(), remote.size());

        std::vector<MatchType> new_matchs;
        new_matchs.reserve(matches.size());

        size_t offset = 0;
        for (auto& match : matches) {
            auto* ptr = local_buffer.data() + offset;
            offset += sizeof(ValueType);

            ValueType value;
            memcpy(&value, ptr, sizeof(ValueType));

            if constexpr (sizeof(Filter) != 1) {
                // filter_complex_expression
                if (filter(match._value, value, match._addr.get())) {
                    match._value = value;
                    new_matchs.emplace_back(std::move(match));
                }
            } else {
                typename Filter::template Comparator<ValueType> comparator { match._value };

                if (comparator(value)) {
                    match._value = value;
                    new_matchs.emplace_back(std::move(match));
                }
            }
        }

        matches = std::move(new_matchs);
    }

    template <typename Filter>
    void filter()
    {
        static_assert(sizeof(Filter) == 1, "see filter_complex_expression");

#define __FILTER(t)                                           \
    if constexpr (IsSuitableFilter<Filter, type##t>::value) { \
        filter<Filter>(_matches_##t);                         \
    }

        MATCH_TYPES(__FILTER);
#undef __FILTER
    }

    template <typename Filter, typename M>
    void filter(M& matches, uintptr_t constant1, uintptr_t constant2)
    {
        typedef typename M::value_type MatchType;
        typedef typename MatchType::type ValueType;

        if (matches.empty()) {
            return;
        }

        std::vector<struct iovec> remote {};
        std::vector<struct iovec> local {};

        remote.reserve(matches.size());
        local.reserve(matches.size());

        for (auto& match : matches) {
            remote.emplace_back(iovec { reinterpret_cast<void*>(match._addr.get()), sizeof(match._value) });
            local.emplace_back(iovec { &match._value, sizeof(match._value) });
        }

        _process->read(local.data(), local.size(), remote.data(), remote.size());

        std::vector<MatchType> new_matchs;
        new_matchs.reserve(matches.size());

        for (auto& match : matches) {
            auto comparator = Filter::template create<ValueType>(constant1, constant2);

            if (comparator(match._value)) {
                new_matchs.emplace_back(std::move(match));
            }
        }
        matches = std::move(new_matchs);
    }

    template <typename Filter>
    void filter(uintptr_t constant1, uintptr_t constant2)
    {
#define __FILTER(t)                                           \
    if constexpr (IsSuitableFilter<Filter, type##t>::value) { \
        filter<Filter>(_matches_##t, constant1, constant2);   \
    }

        MATCH_TYPES(__FILTER);
#undef __FILTER
    }

    template <typename Code>
    void filter_complex_expression(Code& signed_code, Code& unsigned_code)
    {
        static_assert(sizeof(Code) != 1, "see filter_complex_expression");
#define __FILTER(t) \
    filter<Code>(_matches_##t, std::is_signed<type##t>::value ? signed_code : unsigned_code);

        MATCH_TYPES_INTEGER(__FILTER);
#undef __FILTER
    }

    template <typename M>
    void update_matches(M& matches)
    {
        typedef typename M::value_type MatchType;
        typedef typename MatchType::type ValueType;

        if (matches.empty()) {
            return;
        }

        std::vector<struct iovec> remote {};
        std::vector<struct iovec> local {};

        remote.reserve(matches.size());
        local.reserve(matches.size());

        for (auto& match : matches) {
            remote.emplace_back(iovec { reinterpret_cast<void*>(match._addr.get()), sizeof(match._value) });
            local.emplace_back(iovec { &match._value, sizeof(match._value) });
        }

        _process->read(local.data(), local.size(), remote.data(), remote.size());
    }

    void update_matches()
    {
#define __UPDATE(t) \
    update_matches(_matches_##t);

        MATCH_TYPES_INTEGER(__UPDATE);
#undef __UPDATE
    }
};

template <typename Comparator>
class ScanComparator {
public:
    typedef typename Comparator::Type ValueType;
    typedef typename GetMatchType<ValueType>::type MatchType;

private:
    static_assert(std::is_integral<ValueType>::value or std::is_floating_point<ValueType>::value, "Number only");

    Comparator _comparator;
    size_t _step;

public:
    ScanComparator(Comparator&& comparator, size_t step)
        : _comparator { comparator }
        , _step(step)
    {
        assert(_step > 0);
    }

    constexpr size_t step() const { return _step; }

    template <typename Callback>
    void operator()(VMAddress addr_begin, void* buffer_begin, void* buffer_end, Callback&& callback)
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
        if (step == sizeof(ValueType)) {
            auto begin = reinterpret_cast<uintptr_t>(buffer_begin);
            auto end = reinterpret_cast<uintptr_t>(buffer_end);
            for (uintptr_t iter = begin; iter != end; iter += step) {
                auto value = *reinterpret_cast<ValueType*>(iter);
                auto address = addr_begin + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin));
                if (UNLIKELY(comparator(value))) {
                    callback(MatchType(std::move(address), std::move(value)));
                }
            }

        } else {
            auto begin = reinterpret_cast<uintptr_t>(buffer_begin);
            auto end = reinterpret_cast<uintptr_t>(buffer_end);
            for (uintptr_t iter = begin; iter != end; iter += step) {
                ValueType value;
                memcpy(&value, reinterpret_cast<void*>(iter), sizeof(ValueType));
                auto address = addr_begin + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin));
                if (UNLIKELY(comparator(value))) {
                    callback(MatchType(std::move(address), std::move(value)));
                }
            }
        }
    }
};

template <typename T, typename Callable>
class ScanExpression {
public:
    typedef T ValueType;
    typedef typename GetMatchType<ValueType>::type MatchType;

private:
    Callable _callable;
    size_t _step;

public:
    ScanExpression(Callable&& callable, size_t step)
        : _callable { std::move(callable) }
        , _step(step)
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
            ValueType value;
            memcpy(&value, reinterpret_cast<void*>(iter), sizeof(ValueType));
            auto address = addr_begin + (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(buffer_begin));
            if (UNLIKELY(_callable(0, value, address.get()))) {
                callback(MatchType(std::move(address), std::move(value)));
            }
        }
    }
};

} // namespace mypower

#endif
