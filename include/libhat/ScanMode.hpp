#pragma once

namespace hat {

    enum class scan_mode {
        Search,
        FastFirst,
        AVX2,
        AVX512,
        Neon
    };

    template<scan_mode>
    scan_result find_pattern(std::byte* begin, std::byte* end, signature_view signature);
}
