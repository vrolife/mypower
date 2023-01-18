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
#ifndef __raii_hpp__
#define __raii_hpp__

#include "macros.hpp"

#define MYPOWER_RAII_SIMPLE_OBJECT(ObjectName, ObjectType, __free) \
class ObjectName \
{ \
    ObjectType *_obj{nullptr}; \
 \
public: \
    ObjectName() = default; \
    ObjectName(ObjectName&& other) noexcept { \
        _obj = other.release(); \
    } \
    MYPOWER_NO_COPY(ObjectName); \
    explicit ObjectName(ObjectType *obj) : _obj(obj) { } \
    ~ObjectName() { \
        reset(); \
    } \
     \
    operator ObjectType *() { \
        return _obj; \
    } \
 \
    void reset() noexcept { \
        if (_obj != nullptr) { \
            __free(_obj); \
            _obj = nullptr; \
        } \
    } \
 \
    ObjectType * get() noexcept { return _obj; } \
 \
    ObjectName& operator=(ObjectName&& other) noexcept { \
        reset(); \
        _obj = other.release(); \
        return *this; \
    } \
 \
    ObjectName& operator=(ObjectType* other) noexcept { \
        reset(); \
        _obj = other; \
        return *this; \
    } \
 \
    ObjectType * release() noexcept { \
        auto* tmp = _obj; \
        _obj = nullptr; \
        return tmp; \
    } \
 \
    ObjectType ** address() noexcept { \
        return &_obj; \
    } \
    operator bool() const noexcept { return _obj != nullptr; } \
    ObjectType* operator ->() noexcept { return _obj; } \
    const ObjectType* operator ->() const noexcept { return _obj; } \
};

#define MYPOWER_RAII_SHARED_OBJECT(ObjectName, ObjectType, __ref, __unref) \
class ObjectName \
{ \
    ObjectType* _obj{nullptr}; \
public: \
 \
   ObjectName() = default; \
    explicit ObjectName(ObjectType* obj) : _obj(obj){ } \
 \
    ObjectName(const ObjectName& other) { \
        _obj = other._obj; \
        ref(); \
    } \
 \
    ObjectName(ObjectName&& other)  noexcept { \
        _obj = other.release(); \
    } \
 \
    ~ObjectName() { \
        unref(); \
    } \
 \
    ObjectName& operator=(const ObjectName& other) { \
        unref(); \
        _obj = other._obj; \
        ref(); \
        return *this; \
    } \
 \
    ObjectName& operator=(ObjectName&& other) noexcept { \
        unref(); \
        _obj = other.release(); \
        return *this; \
    } \
 \
    void ref() { \
        if (_obj != nullptr) { \
            __ref(_obj); \
        } \
    } \
 \
     void unref() { \
        if (_obj != nullptr) { \
            __unref(_obj); \
        } \
    } \
 \
    ObjectType* release() noexcept { \
        auto* obj = _obj; \
        _obj = nullptr; \
        return obj; \
    } \
 \
    ObjectType ** address() noexcept { \
        return &_obj; \
    } \
 \
    ObjectType* get() const noexcept { \
        return _obj; \
    } \
    void reset() noexcept { unref(); _obj = nullptr; } \
    void reset(ObjectType* obj) { unref(); _obj = obj; } \
\
    operator bool() noexcept { return _obj != nullptr; } \
    operator ObjectType*() noexcept { return _obj; } \
    ObjectType* operator ->() noexcept { return _obj; } \
}

#endif
