#include <libhat/Defines.hpp>
#ifdef LIBHAT_ARM

#include <libhat/Scanner.hpp>

namespace hat::detail {

    template<>
    scan_result find_pattern<scan_mode::Neon>(std::byte* begin, std::byte* end, signature_view signature) {
        const auto firstByte = vld1q_dup_u8(reinterpret_cast<uint8_t*>(*signature[0]));

        auto vec = reinterpret_cast<uint8x16_t*>(begin);
        const auto n = static_cast<size_t>(end - signature.size() - begin) / sizeof(uint8x16_t);
        const auto e = vec + n;

        for (; vec != e; vec++) {
            const auto cmp = vceqq_u8(firstByte, *vec);
            uint64_t first = vgetq_lane_u64(vreinterpretq_u64_u8(cmp), 0);
            uint64_t second = vgetq_lane_u64(vreinterpretq_u64_u8(cmp), 1);
            if (first || second) {
                // TODO: Extract Mask
            }
        }

        return find_pattern<scan_mode::Single>(begin, end, signature);
    }
}
#endif
