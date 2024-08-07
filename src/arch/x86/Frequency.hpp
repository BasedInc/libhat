#pragma once

namespace hat::detail::x86_64 {

    static constexpr auto p(uint8_t a, uint8_t b) {
        return std::pair{std::byte{a}, std::byte{b}};
    }

    // Top 100 byte pair occurrences on 1 byte alignment
    // Accounts for ~39.7% of all pairs
    static constexpr inline std::array pairs_x1{
        p(0x00, 0x00), p(0x48, 0x8B), p(0xCC, 0xCC), p(0x48, 0x8D), p(0x48, 0x89),
        p(0x00, 0x48), p(0x48, 0x83), p(0x44, 0x24), p(0x01, 0x00), p(0x49, 0x8B),
        p(0x48, 0x85), p(0x4C, 0x24), p(0xFF, 0xFF), p(0x0F, 0x11), p(0x4C, 0x8B),
        p(0x08, 0x48), p(0x24, 0x20), p(0x5C, 0x24), p(0x01, 0x48), p(0xFF, 0x48),
        p(0x4C, 0x89), p(0x4C, 0x8D), p(0xCC, 0x48), p(0xFF, 0x15), p(0x10, 0x48),
        p(0x24, 0x30), p(0x03, 0x48), p(0x89, 0x44), p(0x00, 0xE8), p(0x90, 0x48),
        p(0x8D, 0x05), p(0x83, 0xC4), p(0xC3, 0xCC), p(0x20, 0x48), p(0x0F, 0x57),
        p(0x30, 0x48), p(0x02, 0x00), p(0xF3, 0x0F), p(0x00, 0x0F), p(0x54, 0x24),
        p(0x85, 0xC9), p(0xC0, 0x0F), p(0x48, 0xC7), p(0x48, 0x81), p(0x85, 0xC0),
        p(0x74, 0x24), p(0x02, 0x48), p(0x89, 0x5C), p(0x0F, 0x10), p(0x83, 0xEC),
        p(0xC9, 0x74), p(0x8D, 0x4D), p(0x24, 0x40), p(0x57, 0xC0), p(0x24, 0x28),
        p(0x8D, 0x4C), p(0x24, 0x38), p(0x00, 0x4C), p(0x8B, 0xCB), p(0x38, 0x48),
        p(0x48, 0x3B), p(0xF8, 0x48), p(0x8D, 0x0D), p(0xC0, 0x48), p(0x04, 0x48),
        p(0x0F, 0x84), p(0x03, 0x00), p(0x00, 0x49), p(0xC3, 0x48), p(0x8B, 0xCF),
        p(0xC0, 0x74), p(0x89, 0x45), p(0x57, 0x48), p(0x40, 0x48), p(0x48, 0x33),
        p(0x24, 0x48), p(0x24, 0x50), p(0x0F, 0xB6), p(0x8D, 0x15), p(0x18, 0x48),
        p(0x28, 0x48), p(0x0F, 0x7F), p(0x7C, 0x24), p(0x8D, 0x54), p(0x8B, 0x40),
        p(0x8B, 0xC8), p(0x8B, 0x01), p(0x8D, 0x8D), p(0xC1, 0x48), p(0x8B, 0x5C),
        p(0xFE, 0x48), p(0x89, 0x74), p(0xC7, 0x44), p(0x66, 0x0F), p(0x83, 0xF8),
        p(0xCB, 0xE8), p(0x24, 0x60), p(0xCC, 0xE8), p(0xC4, 0x20), p(0x8B, 0x4D),
    };
}
