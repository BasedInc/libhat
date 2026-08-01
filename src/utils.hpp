#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace hat::detail {

    struct scan_parameters;

    constexpr std::uintptr_t fast_align_down(const std::uintptr_t address, const std::size_t alignment) {
        return address & ~static_cast<std::uintptr_t>(alignment - 1);
    }

    constexpr std::uintptr_t fast_align_up(const std::uintptr_t address, const std::size_t alignment) {
        return (address + alignment - 1) & ~static_cast<std::uintptr_t>(alignment - 1);
    }

    std::optional<std::size_t> get_optimal_pair(const scan_parameters& params);
    std::optional<std::size_t> get_optimal_byte(const scan_parameters& params);
}
