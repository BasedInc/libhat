#pragma once

// A slightly modified version of MarkHC's signature scanner in CSGOSimple
// https://github.com/spirthack/CSGOSimple/blob/c37d4bc36efe99c621eb288fd34299c1692ee1dd/CSGOSimple/helpers/utils.cpp#L226
namespace UC1 {
    inline std::vector<int> pattern_to_byte(const std::string_view pattern) {
        auto bytes = std::vector<int>{};
        auto start = pattern.data();
        auto end = start + pattern.size();

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            } else {
                bytes.push_back(strtoul(current, const_cast<char**>(&current), 16));
            }
        }
        return bytes;
    }

    inline const std::byte* PatternScan(const std::byte* begin, const std::byte* end, std::span<const int> patternBytes) {
        auto sizeOfImage = static_cast<size_t>(std::distance(begin, end));
        auto scanBytes = std::to_address(begin);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (auto i = 0ul; i < sizeOfImage - s; ++i) {
            bool found = true;
            for (auto j = 0ul; j < s; ++j) {
                if (scanBytes[i + j] != static_cast<std::byte>(d[j]) && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scanBytes[i];
            }
        }
        return nullptr;
    }
}
