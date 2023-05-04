// hello world
#include <iostream>

#include "fbbe/stacktrace.h"

auto main(int argc, char* argv[]) -> int {
    auto current = fbbe::stacktrace::current();
    auto c2 = fbbe::basic_stacktrace<std::allocator<fbbe::stacktrace_entry>>::current();
    std::cout << current << std::endl;
    return 0;
}