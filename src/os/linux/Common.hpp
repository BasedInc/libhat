#pragma once

#include <charconv>
#include <concepts>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <sys/mman.h>

static void iter_mapped_regions(std::predicate<uintptr_t, uintptr_t, uint32_t> auto&& callback) {
    std::ifstream f("/proc/self/maps");
    std::string s;
    while (std::getline(f, s)) {
        const char* it = s.data();
        const char* end = s.data() + s.size();
        std::from_chars_result res{};

        uintptr_t pageBegin;
        res = std::from_chars(it, end, pageBegin, 16);
        if (res.ec != std::errc{} || res.ptr == end) {
            continue;
        }
        it = res.ptr + 1; // +1 to skip the hyphen

        uintptr_t pageEnd;
        res = std::from_chars(it, end, pageEnd, 16);
        if (res.ec != std::errc{} || res.ptr == end) {
            continue;
        }
        it = res.ptr + 1; // +1 to skip the space

        std::string_view remaining{it, end};
        if (remaining.size() >= 3) {
            uint32_t prot = 0;
            if (remaining[0] == 'r') prot |= PROT_READ;
            if (remaining[1] == 'w') prot |= PROT_WRITE;
            if (remaining[2] == 'x') prot |= PROT_EXEC;
            if (!callback(pageBegin, pageEnd, prot)) {
                return;
            }
        }
    }
}
