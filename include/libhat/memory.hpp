#pragma once

#include <cstdint>
#include <type_traits>

namespace hat {

    enum class protection : uint8_t {
        Read    = 0b001,
        Write   = 0b010,
        Execute = 0b100
    };

    constexpr protection operator|(protection lhs, protection rhs) noexcept {
        using U = std::underlying_type_t<protection>;
        return static_cast<protection>(static_cast<U>(lhs) | static_cast<U>(rhs));
    }

    constexpr protection operator&(protection lhs, protection rhs) noexcept {
        using U = std::underlying_type_t<protection>;
        return static_cast<protection>(static_cast<U>(lhs) & static_cast<U>(rhs));
    }

    constexpr protection operator^(protection lhs, protection rhs) noexcept {
        using U = std::underlying_type_t<protection>;
        return static_cast<protection>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
    }

    constexpr protection& operator|=(protection& lhs, const protection rhs) noexcept {
        return lhs = lhs | rhs;
    }

    constexpr protection& operator&=(protection& lhs, const protection rhs) noexcept {
        return lhs = lhs & rhs;
    }

    constexpr protection& operator^=(protection& lhs, const protection rhs) noexcept {
        return lhs = lhs ^ rhs;
    }
}
