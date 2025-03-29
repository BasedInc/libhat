#include <gtest/gtest.h>
#include <libhat/cow.hpp>

#include <array>
#include <locale>
#include <ranges>
#include <algorithm>
#include <memory_resource>

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

        std::ranges::for_each(str.owned(), [](char& c) { c = std::toupper(c); });
        ASSERT_TRUE(str.is_owned());
        ASSERT_EQ(std::string_view{str.viewed()}, "HELLO, WORLD!");

        str.emplace_viewed("Hello, world!");
        str.emplace_owned();
    }

    {
        const std::array arr = {1, 2, 3, 4, 5};
        std::span span = arr;

        hat::cow_span<const int> vec = arr;
        ASSERT_TRUE(vec.is_viewed());
        ASSERT_TRUE(std::ranges::equal(vec.viewed(), arr));

        vec.owned().push_back(6);
        std::ranges::for_each(vec.owned(), [](int& i) { i++; });
        std::array expected{ 2, 3, 4, 5, 6, 7 };
        ASSERT_TRUE(vec.is_owned());
        ASSERT_TRUE(std::ranges::equal(vec.viewed(), expected));

        vec.emplace_viewed(arr);
        vec.emplace_owned();

        hat::cow_span<const int> moved_vec = std::move(vec);
        vec = std::move(moved_vec);

        hat::cow_span<const int> vec_copy;
        vec_copy = vec;

        ASSERT_TRUE(std::ranges::equal(vec.viewed(), vec_copy.viewed()));
    }

    {
        std::pmr::monotonic_buffer_resource mem_resource{ 4096, std::pmr::new_delete_resource() };
        std::pmr::polymorphic_allocator<char> alloc{&mem_resource};

        std::pmr::vector<hat::pmr::cow_string> strings{ alloc };
        strings.emplace_back(hat::in_place_owned, "Hello, world!");
        ASSERT_TRUE(strings.front().is_owned());
        ASSERT_EQ(alloc, strings.front().get_allocator());
    }
}