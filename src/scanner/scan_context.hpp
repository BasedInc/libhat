#pragma once

#include <memory>
#include <type_traits>
#include <utility>

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
}
