#include <gtest/gtest.h>
#include <libhat/cow.hpp>

#include <locale>
#include <ranges>
#include <algorithm>

TEST(CowTest, CowTest) {
    {
        hat::cow_string str = "Hello, world!";
        ASSERT_TRUE(str.is_viewed());
        ASSERT_EQ(str.viewed(), "Hello, world!");

        std::ranges::for_each(str.owned(), [](char& c) { c = std::toupper(c); });
        ASSERT_TRUE(str.is_owned());
        ASSERT_EQ(str.viewed(), "HELLO, WORLD!");

        str.emplace_viewed("Hello, world!");
        str.emplace_owned();
    }

    {
        hat::cow_cstring str = "Hello, world!";
        ASSERT_TRUE(str.is_viewed());
        ASSERT_EQ(std::string_view{str.viewed()}, "Hello, world!");

        //auto cstr = str.viewed().c_str();

        std::ranges::for_each(str.owned(), [](char& c) { c = std::toupper(c); });
        ASSERT_TRUE(str.is_owned());
        ASSERT_EQ(std::string_view{str.viewed()}, "HELLO, WORLD!");

        str.emplace_viewed("Hello, world!");
        str.emplace_owned();
    }

    {
        const std::array arr = {1, 2, 3, 4, 5};
        std::span span = arr;

        hat::cow_vector<const int> vec = arr;
        ASSERT_TRUE(vec.is_viewed());
        ASSERT_TRUE(std::ranges::equal(vec.viewed(), arr));

        vec.owned().push_back(6);
        std::ranges::for_each(vec.owned(), [](int& i) { i++; });
        std::array expected{ 2, 3, 4, 5, 6, 7 };
        ASSERT_TRUE(vec.is_owned());
        for (auto& i : vec.viewed()) {
            std::printf("- %d\n", i);
        }
        ASSERT_TRUE(std::ranges::equal(vec.viewed(), expected));

        vec.emplace_viewed(arr);
        vec.emplace_owned();
    }
}