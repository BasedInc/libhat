#include <libhat/Process.hpp>

namespace hat::process {

    module_t module_at(uintptr_t address) {
        return module_t{address};
    }

    module_t module_at(std::byte* address) {
        return module_at(reinterpret_cast<uintptr_t>(address));
    }
}