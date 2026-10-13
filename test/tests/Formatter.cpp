#include <gtest/gtest.h>
#include <libhat/cow.hpp>
#include <libhat/cstring_view.hpp>
#include <libhat/fixed_string.hpp>
#include <libhat/signature.hpp>
#include <libhat/string_literal.hpp>
#include <format>

using namespace std::literals;
using namespace hat::literals;

TEST(FormatterTest, FixedString) {
    ASSERT_EQ(std::format("{}", hat::fixed_string{"abc"}), "abc"sv);
}

TEST(FormatterTest, StringLiteral) {
    ASSERT_EQ(std::format("{}", "abc"_s), "abc"sv);
}

TEST(FormatterTest, CStringView) {
    ASSERT_EQ(std::format("{}", "abc"_csv), "abc"sv);
}

TEST(FormatterTest, CowString) {
    ASSERT_EQ(std::format("{}", hat::cow_string{"abc"s}), "abc"sv);
    ASSERT_EQ(std::format("{}", hat::cow_string{"abc"sv}), "abc"sv);
}

TEST(FormatterTest, CowCString) {
    ASSERT_EQ(std::format("{}", hat::cow_cstring{"abc"s}), "abc"sv);
    ASSERT_EQ(std::format("{}", hat::cow_cstring{"abc"_csv}), "abc"sv);
}

TEST(FormatterTest, CowSpan) {
#if __cpp_lib_format_ranges >= 202207L
    std::vector data{1, 2, 3};
    ASSERT_EQ(std::format("{}", hat::cow_span<int>{data}), "[1, 2, 3]"sv);
    ASSERT_EQ(std::format("{}", hat::cow_span<int>{std::span{data}}), "[1, 2, 3]"sv);
    // ASSERT_EQ(std::format("{}", hat::cow_writable_span<int>{data}), "[1, 2, 3]"sv);
    // ASSERT_EQ(std::format("{}", hat::cow_writable_span<int>{std::span{data}}), "[1, 2, 3]"sv);
#else
    GTEST_SKIP_("Formatting ranges requires C++23");
#endif
}

TEST(FormatterTest, SignatureElement) {
    ASSERT_EQ(std::format("{}", hat::signature_element{std::byte{0x10}, std::byte{0xF0}}), "1?"sv);
}

TEST(FormatterTest, Signature) {
    constexpr auto sig = "11 22 33"_sig;
    ASSERT_EQ(std::format("{}", hat::signature(sig.begin(), sig.end())), "11 22 33"sv);
    ASSERT_EQ(std::format("{}", hat::signature_view{sig}), "11 22 33"sv);
    ASSERT_EQ(std::format("{}", sig), "11 22 33"sv);
}
