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
#include "dsl.hpp"
#include "scanner.hpp"
#include "scan.hpp"

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

template<typename T>
void search(Session& session, dsl::ComparatorType opr, uintptr_t constant1, uintptr_t constant2, size_t step) {
    switch(opr) {
        case dsl::ComparatorType::EQ_Expr:
            session.search(ScanComparator<ComparatorEqual<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::NE_Expr:
            session.search(ScanComparator<ComparatorNotEqual<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::GT_Expr:
            session.search(ScanComparator<ComparatorGreaterThen<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::GE_Expr:
            session.search(ScanComparator<ComparatorGreaterOrEqual<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::LT_Expr:
            session.search(ScanComparator<ComparatorLessThen<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::LE_Expr:
            session.search(ScanComparator<ComparatorLessOrEqual<T>>{ {static_cast<T>(constant1)}, step });
            break;
        case dsl::ComparatorType::EQ_Mask:
            session.search(ScanComparator<ComparatorMask<T>>{ { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
            break;
        case dsl::ComparatorType::NE_Mask:
            session.search(ScanComparator<ComparatorMask<T, true>>{ { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
            break;
        case dsl::ComparatorType::EQ_Range:
            session.search(ScanComparator<ComparatorRange<T>>{ { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
            break;
        case dsl::ComparatorType::NE_Range:
            session.search(ScanComparator<ComparatorRange<T, true>>{ { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
            break;
        default:
            assert(false && "Fast mode does not support this operator");
    }
}

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
                << "Invalid date type. Example: search --int32 0x123";
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

        auto comparator = dsl::parse_comparator_expression(config._expr);
        bool fast_mode{false};

        switch(comparator._comparator) {
            case dsl::ComparatorType::Boolean:
                break;
            case dsl::ComparatorType::EQ_Expr:
            case dsl::ComparatorType::NE_Expr:
            case dsl::ComparatorType::GT_Expr:
            case dsl::ComparatorType::GE_Expr:
            case dsl::ComparatorType::LT_Expr:
            case dsl::ComparatorType::LE_Expr: {
                if (comparator._constant1.has_value()) {
                    fast_mode = true;
                }
                break;
            }
            case dsl::ComparatorType::EQ_Mask:
            case dsl::ComparatorType::NE_Mask:
            case dsl::ComparatorType::EQ_Range:
            case dsl::ComparatorType::NE_Range: {
                if (comparator._constant1.has_value() and comparator._constant2.has_value()) {
                    fast_mode = true;
                }
                break;
            }
            case dsl::ComparatorType::EQ:
            case dsl::ComparatorType::NE:
            case dsl::ComparatorType::GT:
            case dsl::ComparatorType::GE:
            case dsl::ComparatorType::LT:
            case dsl::ComparatorType::LE:
                message_view->stream() 
                    << "Error:" << style::SetColor(style::ColorError) 
                    << "Scan mode does not support filter expression";
                return nullptr;
            case dsl::ComparatorType::None:
                message_view->stream() 
                    << "Error:" << style::SetColor(style::ColorError) 
                    << "Invalid expression";
                return nullptr;
        }

        if (config._type_bits & MatchTypeBitI8) {
            if (fast_mode) {
                search<int8_t>(
                    view->_session,
                    comparator._comparator,
                    comparator._constant1.value_or(0),
                    comparator._constant2.value_or(0),
                    config._step);
            } else {
                auto code = comparator.compile();
                view->_session.search(ScanExpression<int8_t, dsl::JITCode>{ std::move(code), config._step });
            }
        }
    }
    return view;
}

} // namespace mypower