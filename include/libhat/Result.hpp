#pragma once

#include <variant>

namespace hat {

    template<class E>
    class result_error {
        template<typename T, typename R>
        friend class result;
    public:
        constexpr explicit result_error(const E& t) : error(t) {}
        constexpr explicit result_error(E&& t) : error(std::move(t)) {}

        constexpr result_error(const result_error<E>&) = default;
        constexpr result_error(result_error<E>&&) noexcept = default;
    private:
        E error;
    };

    /// Highly simplified version of C++23's std::expected, using std::variant for the underlying implementation.
    template<class T, class E>
    class result {
    public:
        constexpr result(const T& t) : impl(std::in_place_index<0>, t) {}
        constexpr result(T&& t) : impl(std::in_place_index<0>, std::move(t)) {}

        constexpr result(const result_error<E>& e) : impl(std::in_place_index<1>, e.error) {}
        constexpr result(result_error<E>&& e) : impl(std::in_place_index<1>, std::move(e.error)) {}

        constexpr result(const result<T, E>&) = default;
        constexpr result(result<T, E>&&) noexcept = default;

        [[nodiscard]] constexpr bool has_value() const {
            return impl.index() == 0;
        }

        constexpr T& value() noexcept {
            return std::get<0>(impl);
        }

        constexpr const T& value() const noexcept {
            return std::get<0>(impl);
        }

        constexpr E& error() noexcept {
            return std::get<1>(impl);
        }

        constexpr const E& error() const noexcept {
            return std::get<1>(impl);
        }
    private:
        std::variant<T, E> impl;
    };
}