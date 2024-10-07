#pragma once

#include <bit>
#include <cstdint>

namespace hat::detail {

    constexpr uintptr_t fast_align_down(uintptr_t address, size_t alignment) {
        return address & ~static_cast<uintptr_t>(alignment - 1);
    }

    constexpr uintptr_t fast_align_up(uintptr_t address, size_t alignment) {
        return (address + alignment - 1) & ~static_cast<uintptr_t>(alignment - 1);
    }
}
