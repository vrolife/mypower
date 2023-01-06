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
#ifndef __matchvalue_hpp__
#define __matchvalue_hpp__

#include <cstring>

#include "vmmap.hpp"

namespace mypower {

enum MatchTypeBits {
    MatchTypeBitU8 = 1,
    MatchTypeBitU16 = 2,
    MatchTypeBitU32 = 4,
    MatchTypeBitU64 = 8,
    MatchTypeBitI8 = 16,
    MatchTypeBitI16 = 32,
    MatchTypeBitI32 = 64,
    MatchTypeBitI64 = 128,
    MatchTypeBitFLOAT = 256,
    MatchTypeBitDOUBLE = 512,
    MatchTypeBitBYTES = 1024,
    MatchTypeBitIntegerMask = 0xFF
};

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

} // namespace mypower

#endif
