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
#ifndef __cmd_scan_hpp__
#define __cmd_scan_hpp__

#include "mypower.hpp"

namespace mypower {

struct ScanArgs {
    std::string _name;
    std::string _expr;
    size_t _step { 0 };
    uint32_t _type_bits { 0 };
    bool _c_string { false };
    bool _suspend_same_user { false };
    uint32_t _prot{kRegionFlagRead};
    bool _exclude_file{false};
};

std::shared_ptr<SessionView> scan(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<Process>& process,
    ScanArgs& config);

bool filter(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<SessionView>& session_view,
    ScanArgs& config);

} // namespace mypower

#endif
