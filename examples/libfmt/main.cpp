#include <fmt/format.h>
#include <fmt/ranges.h>
#include <libhat.hpp>

template<typename T, typename CharT>
    requires(hat::formattable<T, CharT, fmt::formatter>)
struct fmt::formatter<T, CharT> : hat::formatter<T, CharT, fmt::formatter> {};

template<typename T, typename CharT>
    requires(hat::disable_range_formatter<T>)
struct fmt::is_range<T, CharT> : std::false_type {};

int main() {
    using namespace std::literals;
    using namespace hat::literals;

    fmt::println("{}", hat::fixed_string{"abc"});

    fmt::println("{}", "abc"_s);

    fmt::println("{}", "abc"_csv);

    fmt::println("{}", hat::cow_string{"abc"s});
    fmt::println("{}", hat::cow_string{"abc"sv});

    fmt::println("{}", hat::cow_cstring{"abc"s});
    fmt::println("{}", hat::cow_cstring{"abc"_csv});

    std::vector data{1, 2, 3};
    fmt::println("{}", hat::cow_span<int>{data});
    fmt::println("{}", hat::cow_span<int>{std::span{data}});
    // fmt::println("{}", hat::cow_writable_span<int>{data});
    // fmt::println("{}", hat::cow_writable_span<int>{std::span{data}});

    fmt::println("{}", hat::signature_element{std::byte{0x10}, std::byte{0xF0}});

    constexpr auto sig = "11 22 33"_sig;
    fmt::println("{}", hat::signature(sig.begin(), sig.end()));
    fmt::println("{}", hat::signature_view{sig});
    fmt::println("{}", sig);
}
