#pragma once

#include <algorithm>
#include <string>
#include <string_view>

namespace hat {

    template<typename Char, size_t N, template<size_t> typename Derived>
    struct basic_fixed_string {
        using const_reference   = const Char&;
        using const_pointer     = const Char*;
        using const_iterator    = const_pointer;

        static constexpr auto npos = static_cast<size_t>(-1);

        constexpr basic_fixed_string(std::basic_string_view<Char> str) {
            std::copy_n(str.data(), N, value);
        }

        constexpr basic_fixed_string(const Char (&str)[N + 1]) {
            std::copy_n(str, N, value);
        }

        [[nodiscard]] constexpr const_iterator begin() const {
            return this->c_str();
        }

        [[nodiscard]] constexpr const_iterator end() const {
            return this->begin() + this->size();
        }

        [[nodiscard]] constexpr const_reference operator[](size_t pos) const {
            return this->value[pos];
        }

        [[nodiscard]] constexpr const_reference at(size_t pos) const {
            return this->value[pos];
        }

        [[nodiscard]] constexpr const_reference front() const {
            return this->value[0];
        }

        [[nodiscard]] constexpr const_reference back() const {
            return this->value[size() - 1];
        }

        [[nodiscard]] constexpr const_pointer c_str() const {
            return static_cast<const Char*>(this->value);
        }

        [[nodiscard]] constexpr const_pointer data() const {
            return this->c_str();
        }

        [[nodiscard]] constexpr size_t size() const {
            return N;
        }

        [[nodiscard]] constexpr bool empty() const {
            return this->size() == 0;
        }

        template<
            size_t Pos = 0,
            size_t Count = npos,
            size_t M = (Count == npos ? N - Pos : Count)
        >
        [[nodiscard]] constexpr auto substr() const -> Derived<M> {
            static_assert(Pos + M <= N);
            Char buf[M + 1]{};
            std::copy_n(this->value + Pos, M, buf);
            return buf;
        }

        template<size_t M, size_t K = N + M>
        constexpr auto operator+(const basic_fixed_string<Char, M, Derived>& str) const -> Derived<K> {
            Char buf[K + 1]{};
            std::copy_n(this->value, this->size(), buf);
            std::copy_n(str.value, str.size(), buf + this->size());
            return buf;
        }

        template<size_t M>
        constexpr auto operator+(const Char (&str)[M]) const {
            return *this + Derived<M - 1>{str};
        }

        template<size_t M>
        constexpr bool operator==(const basic_fixed_string<Char, M, Derived>& str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(std::basic_string<Char> str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(std::basic_string_view<Char> str) const {
            return std::equal(this->begin(), this->end(), str.begin(), str.end());
        }

        constexpr bool operator==(const Char* str) const {
            return std::equal(this->begin(), this->end(), str, str + std::char_traits<Char>::length(str));
        }

        constexpr std::basic_string<Char> str() const {
            return {this->begin(), this->end()};
        }

        constexpr std::basic_string_view<Char> to_view() const {
            return {this->begin(), this->end()};
        }

        Char value[N + 1]{};
    };

    #define LIBHAT_DEFINE_FIXED_STRING(name, type)                                                          \
    template<size_t N>                                                                                      \
    struct name : public basic_fixed_string<type, N, name> {                                                \
        using basic_fixed_string<type, N, name>::basic_fixed_string;                                        \
    };                                                                                                      \
    template<size_t N>                                                                                      \
    name(const type(&str)[N]) -> name<N - 1>;                                                               \
    template<size_t N, size_t M>                                                                            \
    constexpr inline auto operator+(const type (&cstr)[N], const basic_fixed_string<type, M, name>& lstr) { \
        return name{cstr} + lstr;                                                                           \
    }

    LIBHAT_DEFINE_FIXED_STRING(fixed_string,    char)
    LIBHAT_DEFINE_FIXED_STRING(wfixed_string,   wchar_t)
    LIBHAT_DEFINE_FIXED_STRING(u8fixed_string,  char8_t)
    LIBHAT_DEFINE_FIXED_STRING(u16fixed_string, char16_t)
    LIBHAT_DEFINE_FIXED_STRING(u32fixed_string, char32_t)

    #undef LIBHAT_DEFINE_FIXED_STRING
}
