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
#ifndef __scan_hpp__
#define __scan_hpp__

#include <string>
#include <memory>

#include "process.hpp"
#include "tui.hpp"
#include "matchvalue.hpp"

namespace mypower {

using namespace tui;

class SessionView : public ContentProvider {
    virtual void filter(const std::string_view expr) = 0;
};

struct ScanConfig {
    std::string _expr;
    size_t _step{0};
    uint32_t _type_bits{0};
    uint32_t _memofy_flags{kRegionFlagReadWrite};
};

std::shared_ptr<ContentProvider> scan(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<Process>& process,
    const ScanConfig& config
);

} // namespace mypower

#endif
