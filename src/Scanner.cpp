#include <libhat/Scanner.hpp>

#include <map>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat {

    using namespace hat::process;

    scan_result find_pattern(signature_view signature, module_t mod) {
        const auto data = get_module_data(mod);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern(data.begin(), data.end(), signature);
    }

    scan_result find_pattern(signature_view signature, std::string_view section, module_t mod) {
        const auto data = get_section_data(mod, section);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern(data.begin(), data.end(), signature);
    }
}

namespace hat::detail {

    template<>
    [[deprecated]] scan_result find_pattern<scan_mode::Search>(const std::byte* begin, const std::byte* end, signature_view signature) {
        auto it = std::search(
            begin, end,
            signature.begin(), signature.end(),
            [](auto byte, auto opt) {
                return !opt.has_value() || *opt == byte;
            });
        return it != end ? it : nullptr;
    }

    template<>
    scan_result find_pattern<scan_mode::Auto>(const std::byte* begin, const std::byte* end, signature_view signature) {
        const auto size = signature.size();
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi1 && ext.popcnt) {
            if (size <= 65 && ext.avx512) {
                return find_pattern<scan_mode::AVX512>(begin, end, signature);
            } else if (size <= 33 && ext.avx2) {
                return find_pattern<scan_mode::AVX2>(begin, end, signature);
            } else if (size <= 17 && ext.sse41) {
                return find_pattern<scan_mode::SSE>(begin, end, signature);
            }
        }
#elif defined(LIBHAT_ARM)
        if (size <= 17) {
            return find_pattern<scan_mode::Neon>(begin, end, signature);
        }
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return find_pattern<scan_mode::Single>(begin, end, signature);
    }
}
