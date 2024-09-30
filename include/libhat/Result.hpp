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
        constexpr explicit result_error(const E& t) : error(t) {}
        constexpr explicit result_error(E&& t) : error(std::move(t)) {}

        constexpr result_error(const result_error<E>&) = default;
        constexpr result_error(result_error<E>&&) noexcept = default;
    private:
        E error;
    };

    /// Highly simplified version of C++23's std::expected
    /// If available, std::expected is used as the underlying implementation. Otherwise, std::variant is used.
    template<class T, class E>
    class result {
    public:
#ifdef LIBHAT_RESULT_EXPECTED
        LIBHAT_CONSTEXPR_RESULT result(const T& t) : impl(std::in_place, t) {}
        LIBHAT_CONSTEXPR_RESULT result(T&& t) : impl(std::in_place, std::move(t)) {}

        LIBHAT_CONSTEXPR_RESULT result(const result_error<E>& e) : impl(std::unexpect, e.error) {}
        LIBHAT_CONSTEXPR_RESULT result(result_error<E>&& e) : impl(std::unexpect, std::move(e.error)) {}
#else
        LIBHAT_CONSTEXPR_RESULT result(const T& t) : impl(std::in_place_index<0>, t) {}
        LIBHAT_CONSTEXPR_RESULT result(T&& t) : impl(std::in_place_index<0>, std::move(t)) {}

        LIBHAT_CONSTEXPR_RESULT result(const result_error<E>& e) : impl(std::in_place_index<1>, e.error) {}
        LIBHAT_CONSTEXPR_RESULT result(result_error<E>&& e) : impl(std::in_place_index<1>, std::move(e.error)) {}
#endif

        LIBHAT_CONSTEXPR_RESULT result(const result<T, E>&) = default;
        LIBHAT_CONSTEXPR_RESULT result(result<T, E>&&) noexcept = default;

        [[nodiscard]] LIBHAT_CONSTEXPR_RESULT bool has_value() const {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.has_value();
#else
            return impl.index() == 0;
#endif
        }

        LIBHAT_CONSTEXPR_RESULT T& value() noexcept {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.value();
#else
            return std::get<0>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT const T& value() const noexcept {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.value();
#else
            return std::get<0>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT E& error() noexcept {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.error();
#else
            return std::get<1>(impl);
#endif
        }

        LIBHAT_CONSTEXPR_RESULT const E& error() const noexcept {
#ifdef LIBHAT_RESULT_EXPECTED
            return impl.error();
#else
            return std::get<1>(impl);
#endif
        }

    private:
#ifdef LIBHAT_RESULT_EXPECTED
        std::expected<T, E> impl;
#else
        std::variant<T, E> impl;
#endif
    };
}
