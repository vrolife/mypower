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
#include <sstream>

#include "vmmap.hpp"

#define MATCH_TYPES_INTEGER(F) \
    F(U8) \
    F(U16) \
    F(U32) \
    F(U64) \
    F(I8) \
    F(I16) \
    F(I32) \
    F(I64)

#define MATCH_TYPES(F) \
    MATCH_TYPES_INTEGER(F) \
    F(FLOAT) \
    F(DOUBLE) \
    F(BYTES)

namespace mypower {

typedef int8_t typeI8;
typedef uint8_t typeU8;
typedef int16_t typeI16;
typedef uint16_t typeU16;
typedef int32_t typeI32;
typedef uint32_t typeU32;
typedef int64_t typeI64;
typedef uint64_t typeU64;
typedef float typeFLOAT;
typedef double typeDOUBLE;
typedef std::pair<uint8_t*, size_t> typeBYTES;

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

union UnionValue {
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
};

template<typename T>
struct GetMatchType;

#define __MATCH(t) \
    struct Match##t { \
        typedef type##t type; \
        VMAddress _addr; \
        type##t _value; \
        Match##t(VMAddress&& addr, type##t && value) \
        : _addr(std::move(addr)), _value(std::move(value)) {} \
    }; \
    template<> \
    struct GetMatchType<type##t> { typedef Match##t type; };

MATCH_TYPES(__MATCH);
#undef __MATCH

#define __TYPE_TO_STRING(t) \
    inline \
    std::string to_string(const type##t &) { \
        return #t; \
    }

MATCH_TYPES(__TYPE_TO_STRING);
#undef __TYPE_TO_STRING

template<typename T>
inline 
typename std::enable_if<
    std::is_integral<typename T::type>::value
    or std::is_floating_point<typename T::type>::value
    , std::string>::
type to_string(const T& match)
{
    std::ostringstream os;
    os << std::hex << "0x" << match._addr.get() << std::dec << " " << to_string(match._value) << ": " << match._value;
    return os.str();
}

inline
std::string to_string(const MatchBYTES& match)
{
    return "TODO convert bytes to hex string";
}

} // namespace mypower

#endif
