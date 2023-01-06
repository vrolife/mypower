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
#include "scanner.hpp"
#include "scan.hpp"
#include "comparator.hpp"

namespace mypower {

class SessionViewImpl : public SessionView
{
public:
    Session _session;

    SessionViewImpl(std::shared_ptr<Process>& process)
    : _session(process, 8 * 1024 * 1024)
    { }

    StyleString tui_title(size_t width) override
    {
        return StyleString::layout("Matches", width, 1, '=', LayoutAlign::Center);
    }

    StyleString tui_item(size_t index, size_t width) override
    {
        return {};
    }

    std::string tui_select(size_t index) override {
        return {};
    }

    size_t tui_count() override {
        return _session.size();
    }

    void filter(std::string_view) override {

    }
};

std::shared_ptr<ContentProvider> scan(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<Process>& process,
    const ScanConfig& config
)
{
    auto view = std::make_shared<SessionViewImpl>(process);
    
    if (config._type_bits & MatchTypeBitIntegerMask) {
        size_t data_size = 0;
        size_t step = config._step;

        if (config._type_bits & (MatchTypeBitI8 | MatchTypeBitU8)) {
            data_size = 1;
        } else if (config._type_bits & (MatchTypeBitI16 | MatchTypeBitU16)) {
            data_size = 2;
        } else if (config._type_bits & (MatchTypeBitI32 | MatchTypeBitU32)) {
            data_size = 4;
        } else if (config._type_bits & (MatchTypeBitI64 | MatchTypeBitU64)) {
            data_size = 8;
        }

        if (data_size == 0) 
        {
            message_view->stream() 
                << "Error:" << style::SetColor(style::ColorError) 
                << "I nvalid date type. Example: search --int32 0x123";
            return nullptr;
        }

        if (step == 0) {
            step = data_size;
            message_view->stream() 
                << "Warning:" << style::SetColor(style::ColorWarning) 
                << "Step size is unspecified. Using data size " 
                << style::SetColor(style::ColorInfo) << data_size << style::ResetStyle() 
                << " bytes";
        }

        if (config._type_bits & MatchTypeBitI8) {
            // view->_session.search(ScanNumber<typename Comparator>);
        }
    }
    return view;
}

} // namespace mypower