#pragma once

#include <variant>

#if __cpp_lib_expected >= 202202L
    #define LIBHAT_CONSTEXPR_RESULT constexpr
    #define LIBHAT_RESULT_EXPECTED
    #include <expected>
#elif __cpp_lib_variant >= 202106L
    #define LIBHAT_CONSTEXPR_RESULT constexpr
#else
    #define LIBHAT_CONSTEXPR_RESULT inline
#endif

namespace hat {

    template<class E>
    class result_error {
        template<typename T, typename R>
        friend class result;

    public:
        constexpr explicit result_error(const E& t) noexcept(std::is_nothrow_copy_constructible_v<E>)
            : error(t) {}

        constexpr explicit result_error(E&& t) noexcept(std::is_nothrow_move_constructible_v<E>)
            : error(std::move(t)) {}

    private:
        E error;
    };

    /// Highly simplified version of C++23's std::expected
    /// If available, std::expected is used as the underlying implementation. Otherwise, std::variant is used.
    template<class T, class E>
    class result {
#ifdef LIBHAT_RESULT_EXPECTED
        using underlying_type = std::expected<T, E>;
#else
        using underlying_type = std::variant<T, E>;
#endif
        underlying_type impl;

    public:
#ifdef LIBHAT_RESULT_EXPECTED
        LIBHAT_CONSTEXPR_RESULT result(const T& t) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_t, const T&>)
            : impl(std::in_place, t) {}

        LIBHAT_CONSTEXPR_RESULT result(T&& t) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_t, T&&>)
            : impl(std::in_place, std::move(t)) {}

        LIBHAT_CONSTEXPR_RESULT result(const result_error<E>& e) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::unexpect_t, const result_error<E>&>)
            : impl(std::unexpect, e.error) {}

        LIBHAT_CONSTEXPR_RESULT result(result_error<E>&& e) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::unexpect_t, result_error<E>&&>)
            : impl(std::unexpect, std::move(e.error)) {}
#else
        LIBHAT_CONSTEXPR_RESULT result(const T& t) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_index_t<0>, const T&>)
            : impl(std::in_place_index<0>, t) {}

        LIBHAT_CONSTEXPR_RESULT result(T&& t) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_index_t<0>, T&&>)
            : impl(std::in_place_index<0>, std::move(t)) {}

        LIBHAT_CONSTEXPR_RESULT result(const result_error<E>& e) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_index_t<1>, const result_error<E>&>)
            : impl(std::in_place_index<1>, e.error) {}

        LIBHAT_CONSTEXPR_RESULT result(result_error<E>&& e) noexcept(
            std::is_nothrow_constructible_v<underlying_type, std::in_place_index_t<1>, result_error<E>&&>)
            : impl(std::in_place_index<1>, std::move(e.error)) {}
#endif

        LIBHAT_CONSTEXPR_RESULT result(const result<T, E>&) noexcept(
            std::is_nothrow_copy_constructible_v<underlying_type>) = default;

        LIBHAT_CONSTEXPR_RESULT result(result<T, E>&&) noexcept(
            std::is_nothrow_move_constructible_v<underlying_type>) = default;

        [[nodiscard]] LIBHAT_CONSTEXPR_RESULT bool has_value() const noexcept {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.has_value();
#else
            return impl.index() == 0;
#endif
        }

        LIBHAT_CONSTEXPR_RESULT T& value() {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.value();
#else
            return std::get<0>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT const T& value() const {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.value();
#else
            return std::get<0>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT E& error() {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.error();
#else
            return std::get<1>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT const E& error() const {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.error();
#else
            return std::get<1>(impl);
#endif
        }
    };
}

#undef LIBHAT_RESULT_EXPECTED
