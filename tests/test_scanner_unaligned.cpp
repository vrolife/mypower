#include <sys/ptrace.h>

#include <cassert>
#include <iostream>

#include "scanner.hpp"

using namespace mypower;

volatile struct [[gnu::packed]] {
    char padding[4095];
    uint32_t target;
    uint32_t target2;
} data;

int main(int argc, char* argv[])
{
    data.target = 0x109;
    data.target2 = 0x109;

    auto process = std::make_shared<Process>(getpid());

    auto session = std::make_shared<Session>(process, 4096);
    session->update_memory_region();
    session->search(ScanComparator<ComparatorEqual<uint32_t>> { { 0x109u }, 1 });

    assert(session->size() >= 2);

    data.target = 0x200;
    session->filter(ComparatorEqual<uint32_t>(0x200));

    data.target = 0x201;
    session->filter<FilterNotEqual>();

    std::cout << session->size() << std::endl;
    assert(session->size() == 1);
    assert(session->at(0)._addr.get() == reinterpret_cast<uintptr_t>(&data.target));

    return 0;
}
