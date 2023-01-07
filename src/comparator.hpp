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

    ComparatorRange(T max, T min)
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
};

struct FilterNotEqual {
    template <typename T>
    using Comparator = ComparatorNotEqual<T>;
};

struct FilterGreaterThen {
    template <typename T>
    using Comparator = ComparatorGreaterThen<T>;
};

struct FilterGreaterOrEqual {
    template <typename T>
    using Comparator = ComparatorGreaterOrEqual<T>;
};

struct FilterLessThen {
    template <typename T>
    using Comparator = ComparatorLessThen<T>;
};

struct FilterLessOrEqual {
    template <typename T>
    using Comparator = ComparatorLessOrEqual<T>;
};

} // namespace mypower

#endif
