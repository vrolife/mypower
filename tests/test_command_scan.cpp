#include <unistd.h>

#include <iostream>
#include <memory>

#include "tui.hpp"
#include "dsl.hpp"
#include "process.hpp"
#include "scanner.hpp"
#include "mypower.hpp"

#include "cmd_scan.hpp"

using namespace tui;
using namespace dsl;
using namespace mypower;

int main(int argc, char *argv[])
{
    auto target = std::make_shared<int32_t>(24831);

    std::shared_ptr<MessageView> message_view = std::make_shared<MessageView>();
    std::shared_ptr<Process> process = std::make_shared<Process>(::getpid());
    ScanArgs config{};
    config._expr = "=[24830,24835]";
    config._step = 1;
    config._suspend_same_user = false;
    config._type_bits = MatchTypeBitI32;

    auto session_view = mypower::scan(message_view, process, config);

    for (size_t idx = 0; idx < message_view->tui_count(); ++idx) {
        std::cout << message_view->tui_item(idx, SIZE_MAX).string() << std::endl;
    }

    std::cout << "Matches: " << session_view->tui_count() << std::endl;

    assert(session_view->tui_count() > 0);

    *target += 1;
    config._expr = ">";
    filter(message_view, session_view, config);
    assert(session_view->tui_count() > 0);
    
    *target -= 1;
    config._expr = "<";
    filter(message_view, session_view, config);
    assert(session_view->tui_count() > 0);

    *target = 0x203751;
    config._expr = "$new=0x203751";
    filter(message_view, session_view, config);
    assert(session_view->tui_count() == 1);

    return 0;
}
