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
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>

#include <boost/program_options.hpp>

#include "mypower.hpp"

namespace po = boost::program_options;
using namespace std::string_literals;

namespace mypower {

class TestView : public VisibleContainer<ssize_t> {
    bool _auto_increase { false };
    std::thread _thread {};
    std::timed_mutex _mutex {};

    std::vector<uint32_t> _test_values {};

    static void thread_func(TestView* self)
    {
        while (self->_auto_increase) {
            self->_mutex.try_lock();
            self->_mutex.try_lock_for(std::chrono::seconds { 1 });
            self->at(0) += 1;
        }
    }

    void start()
    {
        if (_thread.joinable()) {
            return;
        }
        _thread = std::thread { &TestView::thread_func, this };
    }

    void stop()
    {
        if (not _thread.joinable()) {
            return;
        }
        _auto_increase = false;
        _mutex.unlock();
        _thread.join();
    }

public:
    TestView()
    {
        std::random_device rd {};
        std::uniform_int_distribution<ssize_t> dist(0, 1000);
        emplace_back(dist(rd));
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
        emplace_back(0);
    }

    ~TestView() override
    {
        stop();
    }

    AttributedString tui_title(size_t width) override
    {
        return AttributedString::layout("Test", width, 1, '-', LayoutAlign::Center);
    }

    AttributedString tui_item(size_t index, size_t width) override
    {
        switch (index) {
        case 0: {
            std::ostringstream os {};
            os << "INT32: " << at(0)
               << " 0x" << std::hex << at(0)
               << " 0o" << std::oct << at(0);
            return AttributedString { os.str() };
        }
        case 1:
            return AttributedString { "+1" };
        case 2:
            return AttributedString { "-1" };
        case 3:
            return AttributedString { "+5" };
        case 4:
            return AttributedString { "-5" };
        case 5:
            if (_auto_increase) {
                return AttributedString { "Auto increase enabled" };
            } else {
                return AttributedString { "Auto increase disabled" };
            }
        }
        return {};
    }

    std::string tui_select(size_t index) override
    {
        switch (index) {
        case 0: {
            std::ostringstream os;
            os << "scan -i -s 4 =" << at(0);
            return os.str();
        }
        case 1:
            at(0) += 1;
            tui_notify_changed();
            break;
        case 2:
            at(0) -= 1;
            tui_notify_changed();
            break;
        case 3:
            at(0) += 5;
            tui_notify_changed();
            break;
        case 4:
            at(0) -= 5;
            tui_notify_changed();
            break;
        case 5: {
            _auto_increase = !_auto_increase;
            if (_auto_increase) {
                start();
            } else {
                stop();
            }
            break;
        }
        }
        return {};
    }

    bool tui_show(size_t width) override
    {
        if (_test_values.empty()) {
            _test_values.resize(100);
            std::fill(_test_values.begin(), _test_values.end(), 0x12341234);
        }
        return true;
    }

    int tui_timeout() override
    {
        this->tui_notify_changed();
        return 1000;
    }
};

class CommandTest : public Command {
    std::shared_ptr<TestView> _test_view;

    po::options_description _options { "Allowed options" };
    po::positional_options_description _posiginal {};

public:
    CommandTest(Application& app)
        : Command(app, "test")
    {
        _options.add_options()("help", "show help message");
        _options.add_options()("value", po::value<ssize_t>(), "set value");
        _posiginal.add("value", 1);

        _test_view = std::make_shared<TestView>();
    }

    void show_short_help() override
    {
        message() << "test\t\t\tShow test view";
    }

    void run(const std::string& command, const std::vector<std::string>& arguments) override
    {
        PROGRAM_OPTIONS();
        if (opts.count("help")) {
            message() << "Usage: " << command << " value\n"
                      << _options;
            show();
            return;
        }
        try {
            if (opts.count("value")) {
                _test_view->at(0) = opts["value"].as<ssize_t>();
            }
        } catch (const std::exception& e) {
            message()
                << EnableStyle(AttrUnderline) << SetColor(ColorError) << "Error: " << ResetStyle()
                << e.what();
            show();
            return;
        }

        show(_test_view);
    }
};

static RegisterCommand<CommandTest> _Test {};

} // namespace mypower
