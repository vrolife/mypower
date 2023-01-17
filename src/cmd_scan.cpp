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
#include <boost/program_options.hpp>

#include "cmd_scan.hpp"
#include "dsl.hpp"
#include "mypower.hpp"
#include "scanner.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

class SuspendProcess {
    std::shared_ptr<Process> _process;
    bool _same_user;

public:
    SuspendProcess(std::shared_ptr<Process>& process, bool same_user = false, bool enable = true)
        : _process(process)
        , _same_user(same_user)
    {
        if (not enable) {
            _process = nullptr;
        }
        if (_process) {
            _process->suspend(same_user);
        }
    }

    ~SuspendProcess()
    {
        if (_process) {
            _process->resume(_same_user);
        }
    }
};

class RefreshView {
    std::shared_ptr<ContentProvider> _view;

public:
    template <typename T>
    RefreshView(T& view, bool enable = true)
        : _view(std::dynamic_pointer_cast<ContentProvider>(view))
    {
        if (not enable) {
            _view = nullptr;
        }
    }

    ~RefreshView()
    {
        if (_view) {
            _view->tui_notify_changed();
        }
    }
};

class SessionViewImpl : public SessionView {
    std::string _name;
    std::string _expr;
    char _mode{'d'};

public:
    Session _session;

    SessionViewImpl(
        std::shared_ptr<Process>& process,
        const std::string& name,
        const std::string& expr)
        : _session(process, 8 * 1024 * 1024)
        , _name(name)
        , _expr(expr)
    {
    }

    const std::string session_name() override
    {
        return _name;
    }

    void session_name(const std::string& name) override
    {
        _name = name;
    }

    void session_reset() override
    {
        _session.reset();
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Matches: "s + _name + " #"s + std::to_string(_session.size()), width, 1, '+', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        // 0xHHHHHHHHHHHH TYPE: DEC HEX REGION
        using namespace tui::attributes;
        AttributedStringBuilder builder {};

        auto access = _session.access(index);
        // std::ostringstream oss;
        builder << std::hex << SetColor(ColorInfo) << "0x" << access->address().get() << " ";
        builder.stream([&](std::ostringstream& oss) { access->type(oss); });
        builder << ResetStyle();
        builder << ": ";

        if (_mode == 'd') {
            builder << "Dec " << std::dec << SetColor(ColorPrompt);
            builder.stream([&](std::ostringstream& oss) { access->value(oss); });
            builder << ResetStyle();
        }

        if (_mode == 'h') {
            builder << "Hex "
                    << SetColor(ColorPrompt)
                    << std::hex << "0x";
            builder.stream([&](std::ostringstream& oss) { access->value(oss); });
            builder << ResetStyle();
        }

        builder << "  Region ";

        _session.find_region(access->address(), [&](VMRegion& region) {
            builder.stream([&](std::ostringstream& oss) { region.string(oss); });
        });

        return builder.release();
    }

    std::string tui_select(size_t index) override
    {
        return {};
    }

    size_t tui_count() override
    {
        return _session.size();
    }

    bool tui_key(int key) override {
        switch(key) {
            case 'h':
                _mode = 'h';
                this->tui_notify_changed();
                return true;
            case 'd':
                _mode = 'd';
                this->tui_notify_changed();
                return true;
            case 'r':
                this->_session.update_matches();
                this->tui_notify_changed();
                return true;
        }
        return false;
    }
};

template <typename T>
static void scan_fast(Session& session, dsl::ComparatorType opr, uintptr_t constant1, uintptr_t constant2, size_t step)
{
    switch (opr) {
    case dsl::ComparatorType::EQ_Expr:
        session.scan(ScanComparator<ComparatorEqual<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::NE_Expr:
        session.scan(ScanComparator<ComparatorNotEqual<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::GT_Expr:
        session.scan(ScanComparator<ComparatorGreaterThen<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::GE_Expr:
        session.scan(ScanComparator<ComparatorGreaterOrEqual<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::LT_Expr:
        session.scan(ScanComparator<ComparatorLessThen<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::LE_Expr:
        session.scan(ScanComparator<ComparatorLessOrEqual<T>> { { static_cast<T>(constant1) }, step });
        break;
    case dsl::ComparatorType::EQ_Mask:
        session.scan(ScanComparator<ComparatorMask<T>> { { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
        break;
    case dsl::ComparatorType::NE_Mask:
        session.scan(ScanComparator<ComparatorMask<T, true>> { { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
        break;
    case dsl::ComparatorType::EQ_Range:
        session.scan(ScanComparator<ComparatorRange<T>> { { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
        break;
    case dsl::ComparatorType::NE_Range:
        session.scan(ScanComparator<ComparatorRange<T, true>> { { static_cast<T>(constant1), static_cast<T>(constant2) }, step });
        break;
    default:
        assert(false && "Fast mode does not support this operator");
    }
}

template <typename T>
static void scan(const ScanArgs& config, bool fast_mode, Session& session, dsl::ComparatorExpression& comparator)
{
    if (fast_mode) {
        scan_fast<T>(
            session,
            comparator._comparator,
            comparator._constant1.value_or(0),
            comparator._constant2.value_or(0),
            config._step);

    } else { // JIT
        auto code = comparator.compile();
        session.scan(ScanExpression<T, dsl::JITCode> { std::move(code), config._step });
    }
}

std::shared_ptr<SessionView> scan(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<Process>& process,
    ScanArgs& config)
{
    auto view = std::make_shared<SessionViewImpl>(process, config._name, config._expr);

    // SuspendProcess suspend{process, config._suspend_same_user, process->pid() != ::getpid()};

    view->_session.update_memory_region();

    if (config._type_bits & MatchTypeBitIntegerMask) {
        size_t data_size = 0;

        if (config._type_bits & (MatchTypeBitI8 | MatchTypeBitU8)) {
            data_size = 1;
        } else if (config._type_bits & (MatchTypeBitI16 | MatchTypeBitU16)) {
            data_size = 2;
        } else if (config._type_bits & (MatchTypeBitI32 | MatchTypeBitU32)) {
            data_size = 4;
        } else if (config._type_bits & (MatchTypeBitI64 | MatchTypeBitU64)) {
            data_size = 8;
        }

        if (data_size == 0) {
            message_view->stream()
                << attributes::SetColor(attributes::ColorError)
                << "Error:"
                << attributes::ResetStyle()
                << " Invalid date type. Example: search --int32 0x123";
            return nullptr;
        }

        if (config._step == 0) {
            config._step = data_size;
            message_view->stream()
                << attributes::SetColor(attributes::ColorWarning)
                << "Warning:"
                << attributes::ResetStyle()
                << " Step size is unspecified. Using data size "
                << attributes::SetColor(attributes::ColorInfo) << data_size << attributes::ResetStyle()
                << " bytes";
        }

        auto comparator = dsl::parse_comparator_expression(config._expr);
        bool fast_mode { false };

        switch (comparator._comparator) {
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
                << attributes::SetColor(attributes::ColorError)
                << "Error:"
                << attributes::ResetStyle()
                << " Scan mode does not support filter expression";
            return nullptr;
        case dsl::ComparatorType::None:
            message_view->stream()
                << attributes::SetColor(attributes::ColorError)
                << "Error:"
                << attributes::ResetStyle()
                << " Invalid expression";
            return nullptr;
        }

        if (config._type_bits & MatchTypeBitI8) {
            scan<int8_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitU8) {
            scan<uint8_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitI16) {
            scan<int16_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitU16) {
            scan<uint16_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitI32) {
            scan<int32_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitU32) {
            scan<uint32_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitI64) {
            scan<int64_t>(config, fast_mode, view->_session, comparator);
        }

        if (config._type_bits & MatchTypeBitU64) {
            scan<uint64_t>(config, fast_mode, view->_session, comparator);
        }
    } else {
        message_view->stream()
            << attributes::SetColor(attributes::ColorError)
            << "Error:"
            << attributes::ResetStyle()
            << " Unsupported data type";
        return nullptr;
    }
    view->tui_notify_changed();
    return view;
}

static void filter_fast(Session& session, dsl::ComparatorType comparator, uintptr_t constant1, uintptr_t constant2)
{
    switch (comparator) {
    case dsl::ComparatorType::EQ_Expr:
        session.filter<FilterEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::NE_Expr:
        session.filter<FilterNotEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::GT_Expr:
        session.filter<FilterGreaterThen>(constant1, constant2);
        break;
    case dsl::ComparatorType::GE_Expr:
        session.filter<FilterGreaterOrEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::LT_Expr:
        session.filter<FilterLessThen>(constant1, constant2);
        break;
    case dsl::ComparatorType::LE_Expr:
        session.filter<FilterLessOrEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::EQ_Mask:
        session.filter<FilterMaskEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::NE_Mask:
        session.filter<FilterMaskNotEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::EQ_Range:
        session.filter<FilterRangeEqual>(constant1, constant2);
        break;
    case dsl::ComparatorType::NE_Range:
        session.filter<FilterRangeNotEqual>(constant1, constant2);
        break;
    default:
        assert(false && "Fast mode does not support this operator");
    }
}

bool filter(
    std::shared_ptr<MessageView>& message_view,
    std::shared_ptr<SessionView>& session_view,
    ScanArgs& config)
{
    RefreshView refresh(session_view);
    auto* view = dynamic_cast<SessionViewImpl*>(session_view.get());

    auto comparator = dsl::parse_comparator_expression(config._expr);
    bool fast_mode { false };

    switch (comparator._comparator) {
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
    case dsl::ComparatorType::NE_Range:
        if (comparator._constant1.has_value() and comparator._constant2.has_value()) {
            fast_mode = true;
        }
        break;
    case dsl::ComparatorType::EQ:
        view->_session.filter<FilterEqual>();
        return true;
    case dsl::ComparatorType::NE:
        view->_session.filter<FilterNotEqual>();
        return true;
    case dsl::ComparatorType::GT:
        view->_session.filter<FilterGreaterThen>();
        return true;
    case dsl::ComparatorType::GE:
        view->_session.filter<FilterGreaterOrEqual>();
        return true;
    case dsl::ComparatorType::LT:
        view->_session.filter<FilterLessThen>();
        return true;
    case dsl::ComparatorType::LE:
        view->_session.filter<FilterLessOrEqual>();
        return true;
    case dsl::ComparatorType::None:
        message_view->stream()
            << attributes::SetColor(attributes::ColorError)
            << "Error:"
            << attributes::ResetStyle()
            << " Invalid expression";
        return false;
    }

    if (fast_mode) {
        filter_fast(
            view->_session,
            comparator._comparator,
            comparator._constant1.value_or(0),
            comparator._constant2.value_or(0));

    } else { // JIT
        if (view->_session.DOUBLE_size()
            or view->_session.FLOAT_size()
            or view->_session.BYTES_size()) {
            message_view->stream()
                << attributes::SetColor(attributes::ColorWarning)
                << "Warning:"
                << attributes::ResetStyle()
                << " Complex filter expression will not be apply to non-integeral matches";
        }
        auto signed_code = comparator.compile(false);
        auto unsigned_code = comparator.compile(true);
        view->_session.filter_complex_expression(signed_code, unsigned_code);
    }
    return true;
}

class CommandScan : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandScan(Application& app)
        : Command(app, "scan")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("step,s", po::value<size_t>(), "step size");
        _options.add_options()("I64,q", po::bool_switch()->default_value(false), "64 bit integer");
        _options.add_options()("I32,i", po::bool_switch()->default_value(false), "32 bit integer");
        _options.add_options()("I16,h", po::bool_switch()->default_value(false), "16 bit integer");
        _options.add_options()("I8,b", po::bool_switch()->default_value(false), "8 bit integer");
        _options.add_options()("U64,Q", po::bool_switch()->default_value(false), "64 bit unsigned integer");
        _options.add_options()("U32,I", po::bool_switch()->default_value(false), "32 bit unsigned integer");
        _options.add_options()("U16,H", po::bool_switch()->default_value(false), "16 bit unsigned integer");
        _options.add_options()("U8,B", po::bool_switch()->default_value(false), "8 bit unsigned integer");
        _options.add_options()("expr", po::value<std::string>(), "scan expression");
        _options.add_options()("name,n", po::value<std::string>(), "session name");
        _posiginal.add("expr", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "scan";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        ScanArgs config;

        try {
            if (opts.count("expr")) {
                config._expr = opts["expr"].as<std::string>();
            }

            if (opts.count("name")) {
                config._name = opts["name"].as<std::string>();
            } else {
                config._name = config._expr;
            }

            config._type_bits |= opts["I8"].as<bool>() ? MatchTypeBitI8 : 0;
            config._type_bits |= opts["I16"].as<bool>() ? MatchTypeBitI16 : 0;
            config._type_bits |= opts["I32"].as<bool>() ? MatchTypeBitI32 : 0;
            config._type_bits |= opts["I64"].as<bool>() ? MatchTypeBitI64 : 0;
            config._type_bits |= opts["U8"].as<bool>() ? MatchTypeBitU8 : 0;
            config._type_bits |= opts["U16"].as<bool>() ? MatchTypeBitU16 : 0;
            config._type_bits |= opts["U32"].as<bool>() ? MatchTypeBitU32 : 0;
            config._type_bits |= opts["U64"].as<bool>() ? MatchTypeBitU64 : 0;

        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        if (config._expr.empty()) {
            message() << "Usage: " << command << " [options] expression\n"
                      << _options;
            show();
            return;
        }

        if (not _app._process) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Invalid target process, attach to target using the 'attach' command";
            show();
            return;
        }

        try {
            auto view = scan(_app._message_view, _app._process, config);
            if (view) {
                _app._session_views.emplace_back(view);
                _app._current_session_view = view;
                show(view);
            }
        } catch (const std::exception& e) {
            message()
                << SetColor(ColorError) << "Error:" << ResetStyle() << " "
                << e.what();
            show();
            return;
        }
    }
};

class CommandFilter : public Command {
    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandFilter(Application& app)
        : Command(app, "filter")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("expr,f", po::value<std::string>(), "filter expression");
        _posiginal.add("expr", 1);
    }

    bool match(const std::string& command) override
    {
        return command == "filter";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();

        ScanArgs config;

        try {
            if (opts.count("expr")) {
                config._expr = opts["expr"].as<std::string>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            return;
        }

        if (config._expr.empty()) {
            message() << "Usage: " << command << " [options] expression\n"
                      << _options;
            show();
            return;
        }

        if (_app._current_session_view == nullptr) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " No avaliable session, create a session using the 'scan' command";
            show();
            return;
        }

        if (not _app._process) {
            message()
                << SetColor(ColorError)
                << "Error:"
                << ResetStyle()
                << " Invalid target process: attach to target using the 'attach' command";
            show();
            return;
        }

        try {
            filter(_app._message_view, _app._current_session_view, config);
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error:" << ResetStyle() << " "
                << e.what();
            show();
            return;
        }
    }
};

static RegisterCommand<CommandScan> _Scan {};

static RegisterCommand<CommandFilter> _Filter {};

} // namespace mypower
