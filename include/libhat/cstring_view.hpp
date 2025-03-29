#pragma once

#include <string>
#include <string_view>

namespace hat {

    inline constexpr struct null_terminated_t{} null_terminated{};

    /// Wrapper around std::basic_string_view with a null-terminator invariant
    template<typename CharT, typename Traits = std::char_traits<CharT>>
    class basic_cstring_view {
        using impl_t = std::basic_string_view<CharT, Traits>;
    public:
        using traits_type            = typename impl_t::traits_type;
        using value_type             = typename impl_t::value_type;
        using pointer                = typename impl_t::pointer;
        using const_pointer          = typename impl_t::const_pointer;
        using reference              = typename impl_t::reference;
        using const_reference        = typename impl_t::const_reference;
        using const_iterator         = typename impl_t::const_iterator;
        using iterator               = typename impl_t::iterator;
        using const_reverse_iterator = typename impl_t::const_reverse_iterator;
        using reverse_iterator       = typename impl_t::reverse_iterator;
        using size_type              = typename impl_t::size_type;
        using difference_type        = typename impl_t::difference_type;

        static constexpr auto npos = impl_t::npos;

        constexpr basic_cstring_view() noexcept = default;
        constexpr basic_cstring_view(const CharT* const c_str) noexcept(noexcept(impl_t{c_str})) : impl(c_str) {}
        constexpr basic_cstring_view(null_terminated_t, const CharT* const c_str, size_t size)
            noexcept(noexcept(impl_t{c_str, size})) : impl(c_str, size) {}
        constexpr basic_cstring_view(const std::basic_string<CharT, Traits>& str) noexcept : impl(str) {}
        constexpr basic_cstring_view(const basic_cstring_view&) noexcept = default;
        basic_cstring_view(std::nullptr_t) = delete;

        constexpr basic_cstring_view& operator=(const basic_cstring_view&) noexcept = default;

        [[nodiscard]] constexpr const_iterator begin() const noexcept {
            return this->impl.begin();
        }

        [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
            return this->impl.cbegin();
        }

        [[nodiscard]] constexpr const_iterator end() const noexcept {
            return this->impl.end();
        }

        [[nodiscard]] constexpr const_iterator cend() const noexcept {
            return this->impl.cend();
        }

        [[nodiscard]] constexpr const_iterator rbegin() const noexcept {
            return this->impl.rbegin();
        }

        [[nodiscard]] constexpr const_iterator crbegin() const noexcept {
            return this->impl.crbegin();
        }

        [[nodiscard]] constexpr const_iterator rend() const noexcept {
            return this->impl.rend();
        }

        [[nodiscard]] constexpr const_iterator crend() const noexcept {
            return this->impl.crend();
        }

        [[nodiscard]] constexpr const_reference operator[](const size_type pos) const {
            return this->impl[pos];
        }

        [[nodiscard]] constexpr const_reference at(const size_type pos) const {
            return this->impl.at(pos);
        }

        [[nodiscard]] constexpr const_reference front() const {
            return this->impl.front();
        }

        [[nodiscard]] constexpr const_reference back() const {
            return this->impl.back();
        }

        [[nodiscard]] constexpr const_pointer data() const noexcept {
            return this->impl.data();
        }

        [[nodiscard]] constexpr const_pointer c_str() const noexcept {
            return this->impl.data();
        }

        [[nodiscard]] constexpr size_type size() const noexcept {
            return this->impl.size();
        }

        [[nodiscard]] constexpr size_type length() const noexcept {
            return this->impl.length();
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept {
            return this->impl.max_size();
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return this->impl.empty();
        }

        constexpr void remove_prefix(const size_type n) {
            this->impl.remove_prefix(n);
        }

        constexpr void swap(basic_cstring_view& v) noexcept {
            this->impl.swap(v.impl);
        }

        size_type copy(CharT* const dest, const size_type count, const size_type pos = 0) const {
            return this->impl.copy(dest, count, pos);
        }

        constexpr basic_cstring_view substr(const size_type pos = 0) const {
            return {this->impl.substr(pos)};
        }

        constexpr impl_t substr(const size_type pos, const size_type count) const {
            return this->impl.substr(pos, count);
        }

        [[nodiscard]] constexpr int compare(const impl_t v) const noexcept {
            return this->impl.compare(v);
        }

        [[nodiscard]] constexpr int compare(const size_type pos1, const size_type count1, const impl_t v) const {
            return this->impl.compare(pos1, count1, v);
        }

        [[nodiscard]] constexpr int compare(const size_type pos1, const size_type count1, const impl_t v,
                                            const size_type pos2, const size_type count2) const {
            return this->impl.compare(pos1, count1, v, pos2, count2);
        }

        [[nodiscard]] constexpr int compare(const CharT* const s) const {
            return this->impl.compare(s);
        }

        [[nodiscard]] constexpr int compare(const size_type pos1, const size_type count1, const CharT* const s) const {
            return this->impl.compare(pos1, count1, s);
        }

        [[nodiscard]] constexpr int compare(const size_type pos1, const size_type count1, const CharT* const s,
                                            const size_type count2) const {
            return this->impl.compare(pos1, count1, s, count2);
        }

        [[nodiscard]] constexpr bool starts_with(const impl_t sv) const noexcept {
            return this->impl.starts_with(sv);
        }

        [[nodiscard]] constexpr bool starts_with(const CharT ch) const noexcept {
            return this->impl.starts_with(ch);
        }

        [[nodiscard]] constexpr bool starts_with(const CharT* const s) const {
            return this->impl.starts_with(s);
        }

        [[nodiscard]] constexpr bool ends_with(const impl_t sv) const noexcept {
            return this->impl.ends_with(sv);
        }

        [[nodiscard]] constexpr bool ends_with(const CharT ch) const noexcept {
            return this->impl.ends_with(ch);
        }

        [[nodiscard]] constexpr bool ends_with(const CharT* const s) const {
            return this->impl.ends_with(s);
        }

        [[nodiscard]] constexpr bool contains(const impl_t sv) const noexcept {
            return this->impl.contains(sv);
        }

        [[nodiscard]] constexpr bool contains(const CharT c) const noexcept {
            return this->impl.contains(c);
        }

        [[nodiscard]] constexpr bool contains(const CharT* const s) const {
            return this->impl.contains(s);
        }

        [[nodiscard]] constexpr size_type find(const impl_t v, const size_type pos = 0) const noexcept {
            return this->impl.find(v, pos);
        }

        [[nodiscard]] constexpr size_type find(const CharT ch, const size_type pos = 0) const noexcept {
            return this->impl.find(ch, pos);
        }

        [[nodiscard]] constexpr size_type find(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.find(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find(const CharT* const s, const size_type pos = 0) const {
            return this->impl.find(s, pos);
        }

        [[nodiscard]] constexpr size_type rfind(const impl_t v, const size_type pos = npos) const noexcept {
            return this->impl.rfind(v, pos);
        }

        [[nodiscard]] constexpr size_type rfind(const CharT ch, const size_type pos = npos) const noexcept {
            return this->impl.rfind(ch, pos);
        }

        [[nodiscard]] constexpr size_type rfind(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.rfind(s, pos, count);
        }

        [[nodiscard]] constexpr size_type rfind(const CharT* const s, const size_type pos = npos) const {
            return this->impl.rfind(s, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(const impl_t v, const size_type pos = 0) const noexcept {
            return this->impl.find_first_of(v, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(const CharT ch, const size_type pos = 0) const noexcept {
            return this->impl.find_first_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_first_of(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.find_first_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_first_of(const CharT* const s, const size_type pos = 0) const {
            return this->impl.find_first_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(const impl_t v, const size_type pos = npos) const noexcept {
            return this->impl.find_last_of(v, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(const CharT ch, const size_type pos = npos) const noexcept {
            return this->impl.find_last_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_last_of(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.find_last_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_last_of(const CharT* const s, const size_type pos = npos) const {
            return this->impl.find_last_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const impl_t v, const size_type pos = 0) const noexcept {
            return this->impl.find_first_not_of(v, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const CharT ch, const size_type pos = 0) const noexcept {
            return this->impl.find_first_not_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.find_first_not_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const CharT* const s, const size_type pos = 0) const {
            return this->impl.find_first_not_of(s, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const impl_t v, const size_type pos = npos) const noexcept {
            return this->impl.find_last_not_of(v, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const CharT ch, const size_type pos = npos) const noexcept {
            return this->impl.find_last_not_of(ch, pos);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const CharT* const s, const size_type pos, const size_type count) const {
            return this->impl.find_last_not_of(s, pos, count);
        }

        [[nodiscard]] constexpr size_type find_last_not_of(const CharT* const s, const size_type pos = npos) const {
            return this->impl.find_last_not_of(s, pos);
        }

        constexpr operator impl_t() const noexcept {
            return this->impl;
        }

    private:
        constexpr basic_cstring_view(impl_t impl) noexcept : impl(impl) {}
        impl_t impl{};
    };

    using cstring_view = basic_cstring_view<char>;
    using wcstring_view = basic_cstring_view<wchar_t>;
#ifdef __cpp_lib_char8_t
    using u8cstring_view = basic_cstring_view<char8_t>;
#endif
    using u16cstring_view = basic_cstring_view<char16_t>;
    using u32cstring_view = basic_cstring_view<char32_t>;

    namespace detail {

        template<typename Traits>
        struct comparison_category {
            using type = std::weak_ordering;
        };

        template<typename Traits> requires requires { typename Traits::comparison_category; }
        struct comparison_category<Traits> {
            using type = typename Traits::comparison_category;
        };

        template<typename Traits>
        using comparison_category_t = typename comparison_category<Traits>::type;
    }
}

template<typename CharT, typename Traits>
constexpr hat::detail::comparison_category_t<Traits> operator<=>(
    hat::basic_cstring_view<CharT, Traits> lhs,
    std::type_identity_t<std::basic_string_view<CharT, Traits>> rhs) noexcept
{
    return std::basic_string_view<CharT, Traits>{lhs} <=> rhs;
}

namespace hat::literals::cstring_view_literals {

    [[nodiscard]] constexpr cstring_view operator""_csv(const char* str, size_t size) {
        return {null_terminated, str, size};
    }

    [[nodiscard]] constexpr wcstring_view operator""_csv(const wchar_t* str, size_t size) {
        return {null_terminated, str, size};
    }

#ifdef __cpp_lib_char8_t
    [[nodiscard]] constexpr u8cstring_view operator""_csv(const char8_t* str, size_t size) {
        return {null_terminated, str, size};
    }
#endif

    [[nodiscard]] constexpr u16cstring_view operator""_csv(const char16_t* str, size_t size) {
        return {null_terminated, str, size};
    }

    [[nodiscard]] constexpr u32cstring_view operator""_csv(const char32_t* str, size_t size) {
        return {null_terminated, str, size};
    }
}

template<> struct std::hash<hat::cstring_view> : std::hash<std::string_view> {};
template<> struct std::hash<hat::wcstring_view> : std::hash<std::wstring_view> {};

#ifdef __cpp_lib_char8_t
template<> struct std::hash<hat::u8cstring_view> : std::hash<std::u8string_view> {};
#endif

template<> struct std::hash<hat::u16cstring_view> : std::hash<std::u16string_view> {};
template<> struct std::hash<hat::u32cstring_view> : std::hash<std::u32string_view> {};

template<typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_borrowed_range<hat::basic_cstring_view<CharT, Traits>> = true;

template<typename CharT, typename Traits>
inline constexpr bool std::ranges::enable_view<hat::basic_cstring_view<CharT, Traits>> = true;
