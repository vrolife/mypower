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

#include <sstream>

#include "operators.hpp"
#include "vmmap.hpp"

#if USE_SIMD
#include <simdjson/arm64/simd.h>
#include <simdjson/dom.h>
#include <simdjson/implementations.h>
#endif

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

namespace mypower {

using namespace std::string_literals;

enum class MatchType {
    U8,
    U16,
    U32,
    U64,
    I8,
    I16,
    I32,
    I64,
    FLOAT,
    DOUBLE,
    BYTES,
};

struct MatchValue {
    VMAddress _addr;
    uint64_t _type : 8;
    uint64_t _reserved : 24;
    uint64_t _size : 32;
    union {
        uint8_t _u8;
        uint16_t _u16;
        uint32_t _u32;
        uint64_t _u64;
        int8_t _i8;
        int16_t _i16;
        int32_t _i32;
        int64_t _i64;
        float _float;
        double _double;
        uint8_t _bytes[0];
    } _data;

    std::string type_string() const
    {
        switch (static_cast<MatchType>(_type)) {
        case MatchType::U8:
            return "U8";
        case MatchType::U16:
            return "U16";
        case MatchType::U32:
            return "U32";
        case MatchType::U64:
            return "U64";
        case MatchType::I8:
            return "I8";
        case MatchType::I16:
            return "I16";
        case MatchType::I32:
            return "I32";
        case MatchType::I64:
            return "I64";
        case MatchType::FLOAT:
            return "FLOAT";
        case MatchType::DOUBLE:
            return "DOUBLE";
        case MatchType::BYTES:
            return "BYTES";
        }
        ::abort();
    }

    std::string value_string() const
    {
        switch (static_cast<MatchType>(_type)) {
        case MatchType::U8:
            return std::to_string(_data._u8);
        case MatchType::U16:
            return std::to_string(_data._u16);
        case MatchType::U32:
            return std::to_string(_data._u32);
        case MatchType::U64:
            return std::to_string(_data._u64);
        case MatchType::I8:
            return std::to_string(_data._i8);
        case MatchType::I16:
            return std::to_string(_data._i16);
        case MatchType::I32:
            return std::to_string(_data._i32);
        case MatchType::I64:
            return std::to_string(_data._i64);
        case MatchType::FLOAT:
            return std::to_string(_data._float);
        case MatchType::DOUBLE:
            return std::to_string(_data._double);
        case MatchType::BYTES:
            return to_hex(_data._bytes, _size);
        }
        ::abort();
    }

    template <typename T>
    bool filter(void* ptr)
    {
        switch (static_cast<MatchType>(_type)) {
#define __COMPARE(t, dt, mt)                                     \
    case MatchType::mt: {                                        \
        t value;                                                 \
        memcpy(&value, ptr, _size);                              \
        typename T::template Comparator<t> comparator { value }; \
        return comparator(_data._##dt);                          \
    }
            __COMPARE(uint8_t, u8, U8)
            __COMPARE(uint16_t, u16, U16)
            __COMPARE(uint32_t, u32, U32)
            __COMPARE(uint64_t, u64, U64)
            __COMPARE(int8_t, i8, I8)
            __COMPARE(int16_t, i16, I16)
            __COMPARE(int32_t, i32, I32)
            __COMPARE(int64_t, i64, I64)

            __COMPARE(float, float, FLOAT)
            __COMPARE(double, double, DOUBLE)
        case MatchType::BYTES: {
            typename T::template Comparator<uint8_t*> comparator { reinterpret_cast<uint8_t*>(ptr), _size };
            return comparator(_data._bytes);
        }
        }
#undef __COMPARE
        ::abort();
    }

    template <typename Comparator>
    bool filter(Comparator&& comparator)
    {
        switch (static_cast<MatchType>(_type)) {
#define __COMPARE(t, dt, mt)            \
    case MatchType::mt: {               \
        return comparator(_data._##dt); \
    }
            __COMPARE(uint8_t, u8, U8)
            __COMPARE(uint16_t, u16, U16)
            __COMPARE(uint32_t, u32, U32)
            __COMPARE(uint64_t, u64, U64)
            __COMPARE(int8_t, i8, I8)
            __COMPARE(int16_t, i16, I16)
            __COMPARE(int32_t, i32, I32)
            __COMPARE(int64_t, i64, I64)

            __COMPARE(float, float, FLOAT)
            __COMPARE(double, double, DOUBLE)
            __COMPARE(uint8_t*, bytes, BYTES)
        }
#undef __COMPARE
        ::abort();
    }

    void operator delete(void* ptr)
    {
        ::free(ptr);
    }

    template <typename T>
    static
        typename std::enable_if<std::is_integral<T>::value or std::is_floating_point<T>::value, std::unique_ptr<MatchValue>>::
            type
            create(VMAddress addr, T data)
    {
        void* ptr = malloc(sizeof(MatchValue));
        auto* self = reinterpret_cast<MatchValue*>(ptr);
        self->_addr = addr;
        self->_size = sizeof(data);
        self->_type = assign(self, data);
        return std::unique_ptr<MatchValue> { self };
    }

    static std::unique_ptr<MatchValue> create(VMAddress addr, uint8_t* data, size_t size)
    {
        void* ptr = malloc(offsetof(MatchValue, _data) + size);
        auto* self = reinterpret_cast<MatchValue*>(ptr);
        self->_addr = addr;
        self->_size = size;
        self->_type = static_cast<uint64_t>(MatchType::BYTES);
        memcpy(self->_data._bytes, data, size);
        return std::unique_ptr<MatchValue> { self };
    }

private:
    template <typename T>
    static uint64_t assign(MatchValue* self, T data)
    {
#define __ASSIGN(t, dt, mt)                          \
    if constexpr (std::is_same<T, t>::value) {       \
        self->_data._##dt = data;                    \
        return static_cast<uint64_t>(MatchType::mt); \
    }
        __ASSIGN(uint8_t, u8, U8)
        __ASSIGN(uint16_t, u16, U16)
        __ASSIGN(uint32_t, u32, U32)
        __ASSIGN(uint64_t, u64, U64)
        __ASSIGN(int8_t, i8, I8)
        __ASSIGN(int16_t, i16, I16)
        __ASSIGN(int32_t, i32, I32)
        __ASSIGN(int64_t, i64, I64)

        __ASSIGN(float, float, FLOAT)
        __ASSIGN(float, double, DOUBLE)
#undef __ASSIGN
        ::abort();
    }

    static std::string to_hex(const void* data, size_t len)
    {
        std::ostringstream os;
        for (size_t i = 0; i < len; ++i) {
            os << std::setw(2) << std::setfill('0') << std::hex << (int)reinterpret_cast<const uint8_t*>(data)[i];
        }
        return os.str();
    }
};

class Process {
    pid_t _pid;

public:
    Process(pid_t pid)
        : _pid(pid)
    {
    }

    pid_t pid() const { return _pid; }

    ssize_t read(VMAddress address, void* buffer, size_t size);
    ssize_t write(VMAddress address, const void* buffer, size_t size);
    ssize_t read(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count);
    ssize_t write(struct iovec* local, size_t local_count, struct iovec* remote, size_t remote_count);

    bool suspend(bool same_user = false);
    bool resume(bool same_user = false);
};

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
    Session(std::shared_ptr<Process>& process, size_t cache_size);
    ~Session();

    bool update_memory_region()
    {
        _memory_regions = VMRegion::snapshot(_process->pid());
        return not _memory_regions.empty();
    }

    const MatchValue& at(size_t index) const {
        return *_matchs.at(index);
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

template <typename Comparator, size_t Step>
class ScanNumber {
    typedef typename Comparator::Type Type;
    static_assert(std::is_integral<Type>::value or std::is_floating_point<Type>::value, "Number only");

    Comparator _comparator;

public:
    template <typename... TArgs>
    ScanNumber(TArgs&&... args)
        : _comparator { std::forward<TArgs>(args)... }
    {
        if (this->step() == 0) {
            throw std::logic_error("Invalid step");
        }
    }

    constexpr size_t step() const { return Step; }

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

} // namespace mypower

#endif
