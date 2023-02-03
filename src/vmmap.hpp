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
#ifndef __vmmap_hpp__
#define __vmmap_hpp__

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace mypower {

enum class RegionType {
    Misc,
    Stack,
    Code,
};

enum RegionFlag {
    kRegionFlagNone = 0,
    kRegionFlagRead = 1,
    kRegionFlagWrite = 2,
    kRegionFlagExec = 4,
    kRegionFlagReadWrite = 3,
    kRegionFlagAll = 7
};

class VMAddress {
    uintptr_t _addr;

public:
    explicit VMAddress(uintptr_t addr)
        : _addr(addr)
    {
    }

    VMAddress(const VMAddress&) = default;
    VMAddress(VMAddress&&) noexcept = default;

    uintptr_t get() const
    {
        return _addr;
    }

    explicit operator uintptr_t()
    {
        return _addr;
    }

    VMAddress& operator=(const VMAddress&) = default;
    VMAddress& operator=(VMAddress&&) noexcept = default;

    bool operator==(const VMAddress& other) const
    {
        return _addr == other._addr;
    }

    bool operator!=(const VMAddress& other) const
    {
        return _addr != other._addr;
    }

    bool operator>(const VMAddress& other) const
    {
        return _addr > other._addr;
    }

    bool operator<(const VMAddress& other) const
    {
        return _addr < other._addr;
    }

    bool operator>=(const VMAddress& other) const
    {
        return _addr >= other._addr;
    }

    bool operator<=(const VMAddress& other) const
    {
        return _addr <= other._addr;
    }

    template <typename T>
    typename std::enable_if<std::is_integral<typename std::decay<T>::type>::value, VMAddress>::
        type
        operator+(T&& other) const
    {
        return VMAddress { _addr + other };
    }

    VMAddress operator+(const VMAddress& other) const
    {
        return VMAddress { _addr + other._addr };
    }

    template <typename T>
    typename std::enable_if<std::is_integral<typename std::decay<T>::type>::value, VMAddress>::
        type
        operator-(T&& other) const
    {
        return VMAddress { _addr - other };
    }

    VMAddress operator-(const VMAddress& other) const
    {
        return VMAddress { _addr - other._addr };
    }

    template <typename T>
    VMAddress& operator+=(T&& other)
    {
        _addr += other;
        return *this;
    }

    template <typename T>
    VMAddress& operator-=(T&& other) noexcept
    {
        _addr -= other;
        return *this;
    }
};

struct VMRegion {
    VMAddress _begin { 0 };
    VMAddress _end { 0 };
    uint32_t _prot { 0 };
    bool _shared { false };

    std::string _file {};
    std::string _desc {};

    uintptr_t _offset { 0 };
    int _major { 0 };
    int _minor { 0 };
    int _inode { 0 };
    bool _deleted { false };
    bool _android_bss{false};

    VMRegion() = default;

    uintptr_t size() const
    {
        return _end.get() - _begin.get();
    }

    void string(std::ostringstream& oss);

    typedef std::vector<VMRegion> ListType;

    static ListType snapshot(std::istream& is);
    static ListType snapshot(const std::filesystem::path& maps);
    static ListType snapshot(pid_t pid);
};

} // namespace mypower

#endif
