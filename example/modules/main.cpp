#include <bit>
#include <span>
#include <string_view>

import libhat;

int main() {
    using namespace std::literals;

    constexpr auto str = "abcdefghijklmnopqrstuvwxyz0123456789"sv;
    const auto sig = hat::string_to_signature("xyz"sv);
    const auto buf = std::as_bytes(std::span{str});

    const auto result = hat::find_pattern(buf.begin(), buf.end(), sig);
    if (result.has_result()) {
        const uintptr_t offset = std::bit_cast<uintptr_t>(result.get()) - std::bit_cast<uintptr_t>(buf.data());
        printf("Found at %zu\n", offset); // Prints "Found at 23"
    } else {
        printf("Not found\n");
        return 1;
    }
    return 0;
}
