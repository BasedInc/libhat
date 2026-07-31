#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>

#include <libhat/scanner.hpp>

namespace hat::detail {

    template<typename T, typename... Args>
    void scan_context::emplace(Args&&... args) {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>>);
        static_assert(sizeof(T) <= impl_buffer_size);
        static_assert(alignof(T) <= impl_buffer_align);

        std::construct_at(reinterpret_cast<T*>(impl_buffer_.data()), std::forward<Args>(args)...);

        if constexpr (not std::is_trivially_destructible_v<T>) {
            impl_deleter_ = [](const void* buffer) {
                std::destroy_at(static_cast<const T*>(buffer));
            };
        }
    }

    template<typename T>
    const T& scan_context::get() const {
        return *reinterpret_cast<const T*>(impl_buffer_.data());
    }

    constexpr std::uintptr_t fast_align_down(const std::uintptr_t address, const std::size_t alignment) {
        return address & ~static_cast<std::uintptr_t>(alignment - 1);
    }

    constexpr std::uintptr_t fast_align_up(const std::uintptr_t address, const std::size_t alignment) {
        return (address + alignment - 1) & ~static_cast<std::uintptr_t>(alignment - 1);
    }

    std::optional<std::size_t> get_optimal_pair(const scan_parameters& params);
    std::optional<std::size_t> get_optimal_byte(const scan_parameters& params);
}
