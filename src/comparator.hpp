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
#ifndef __comparator_hpp__
#define __comparator_hpp__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "matchvalue.hpp"

namespace mypower {

template <typename T, bool Xor=false>
class ComparatorMask {
    T _target;
    T _mask;

public:
    typedef T Type;

    ComparatorMask(T target, T mask)
        : _target(target & mask)
        , _mask(mask)
    {
    }

    inline bool operator()(const T& value) const
    {
        return ((value & _mask) == _target) ^ Xor;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T, bool Xor=false>
class ComparatorRange {
    T _max;
    T _min;

public:
    typedef T Type;

    ComparatorRange(T min, T max)
        : _max(max)
        , _min(min)
    {
    }

    inline bool operator()(const T& value) const
    {
        return (value >= _min and value <= _max) ^ Xor;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorRange<uint8_t*> {
public:
    inline bool operator()(const uint8_t* value) const
    {
        return true;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorEqual {
    T _rhs;

public:
    typedef T Type;

    ComparatorEqual(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value == _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorEqual<uint8_t*> {
    uint8_t* _rhs;
    size_t _size;

public:
    typedef uint8_t* Type;

    ComparatorEqual(uint8_t* rhs, size_t size)
        : _rhs(rhs)
        , _size(size)
    {
    }

    inline bool operator()(const uint8_t* value) const
    {
        return memcmp(value, _rhs, _size) == 0;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorNotEqual {
    T _rhs;

public:
    typedef T Type;

    ComparatorNotEqual(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value != _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorNotEqual<uint8_t*> {
    uint8_t* _rhs;
    size_t _size;

public:
    typedef uint8_t* Type;

    ComparatorNotEqual(uint8_t* rhs, size_t size)
        : _rhs(rhs)
        , _size(size)
    {
    }

    inline bool operator()(const uint8_t* value) const
    {
        return memcmp(value, _rhs, _size) != 0;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorGreaterThen {
    T _rhs;

public:
    typedef T Type;

    ComparatorGreaterThen(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value > _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorGreaterThen<uint8_t*> {
public:
    ComparatorGreaterThen(const uint8_t*, size_t) { }

    inline bool operator()(const uint8_t* value) const
    {
        return true;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorLessThen {
    T _rhs;

public:
    typedef T Type;

    ComparatorLessThen(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value < _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorLessThen<uint8_t*> {
public:
    ComparatorLessThen(const uint8_t*, size_t) { }

    inline bool operator()(const uint8_t* value) const
    {
        return true;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorGreaterOrEqual {
    T _rhs;

public:
    typedef T Type;

    ComparatorGreaterOrEqual(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value >= _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorGreaterOrEqual<uint8_t*> {
public:
    ComparatorGreaterOrEqual(const uint8_t*, size_t) { }

    inline bool operator()(const uint8_t* value) const
    {
        return true;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <typename T>
class ComparatorLessOrEqual {
    T _rhs;

public:
    typedef T Type;

    ComparatorLessOrEqual(T rhs)
        : _rhs(rhs)
    {
    }

    inline bool operator()(const T& value) const { return value <= _rhs; }

    inline bool operator()(...) const
    {
        return true;
    }
};

template <>
class ComparatorLessOrEqual<uint8_t*> {
public:
    ComparatorLessOrEqual(const uint8_t*, size_t) { }

    inline bool operator()(const uint8_t* value) const
    {
        return true;
    }

    inline bool operator()(...) const
    {
        return true;
    }
};

struct FilterEqual {
    template <typename T>
    using Comparator = ComparatorEqual<T>;

    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterNotEqual {
    template <typename T>
    using Comparator = ComparatorNotEqual<T>;
    
    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterGreaterThen {
    template <typename T>
    using Comparator = ComparatorGreaterThen<T>;
    
    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterGreaterOrEqual {
    template <typename T>
    using Comparator = ComparatorGreaterOrEqual<T>;
    
    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterLessThen {
    template <typename T>
    using Comparator = ComparatorLessThen<T>;
    
    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterLessOrEqual {
    template <typename T>
    using Comparator = ComparatorLessOrEqual<T>;
    
    template<typename T>
    static
    Comparator<T> create(uintptr_t value, ...) {
        return Comparator<T>(static_cast<T>(value));
    }
};

struct FilterMaskEqual {
    template <typename T>
    using Comparator = ComparatorMask<T>;

    template<typename T>
    static
    Comparator<T> create(uintptr_t value, uintptr_t mask) {
        return Comparator<T>(value, mask);
    }
};

struct FilterMaskNotEqual {
    template <typename T>
    using Comparator = ComparatorMask<T, true>;

    template<typename T>
    static
    Comparator<T> create(uintptr_t value, uintptr_t mask) {
        return Comparator<T>(value, mask);
    }
};

struct FilterRangeEqual {
    template <typename T>
    using Comparator = ComparatorRange<T>;

    template<typename T>
    static
    Comparator<T> create(uintptr_t min, uintptr_t max) {
        return Comparator<T>(min, max);
    }
};

struct FilterRangeNotEqual {
    template <typename T>
    using Comparator = ComparatorRange<T, true>;

    template<typename T>
    static
    Comparator<T> create(uintptr_t min, uintptr_t max) {
        return Comparator<T>(min, max);
    }
};

template<typename F, typename T>
struct IsSuitableFilter : std::true_type { };

template<>
struct IsSuitableFilter<FilterMaskEqual, typeFLOAT> : std::false_type { };
template<>
struct IsSuitableFilter<FilterMaskEqual, typeDOUBLE> : std::false_type { };

template<>
struct IsSuitableFilter<FilterMaskNotEqual, typeFLOAT> : std::false_type { };
template<>
struct IsSuitableFilter<FilterMaskNotEqual, typeDOUBLE> : std::false_type { };

template<typename F>
struct IsSuitableFilter<F, typeBYTES> : std::false_type { };

} // namespace mypower

#endif
