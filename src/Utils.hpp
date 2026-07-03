#pragma once

#include <bit>
#include <cstdint>

#include <libhat/scanner.hpp>

namespace hat::detail {

    constexpr uintptr_t fast_align_down(uintptr_t address, size_t alignment) {
        return address & ~static_cast<uintptr_t>(alignment - 1);
    }

    constexpr uintptr_t fast_align_up(uintptr_t address, size_t alignment) {
        return (address + alignment - 1) & ~static_cast<uintptr_t>(alignment - 1);
    }

    template<auto impl>
    auto* find_specialization_switch(const scan_alignment alignment, const bool cmpeq2, const bool veccmp) {
        const auto with_alignment = [&]<scan_alignment A>(std::integral_constant<scan_alignment, A>) {
            if (cmpeq2 && veccmp) return impl.template operator()<A, true, true>();
            if (cmpeq2) return impl.template operator()<A, true, false>();
            if (veccmp) return impl.template operator()<A, false, true>();
            return impl.template operator()<A, false, false>();
        };

        switch (alignment) {
            using enum scan_alignment;
            case X1: return with_alignment(std::integral_constant<scan_alignment, X1>{});
            case X4: return with_alignment(std::integral_constant<scan_alignment, X4>{});
            case X16: return with_alignment(std::integral_constant<scan_alignment, X16>{});
        }
        LIBHAT_UNREACHABLE();
    }
}
