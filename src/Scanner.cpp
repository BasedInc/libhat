#include <libhat/Scanner.hpp>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat {

    using namespace hat::process;

    template<scan_alignment alignment>
    scan_result find_pattern(signature_view signature, module_t mod) {
        const auto data = get_module_data(mod);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern<alignment>(data.begin(), data.end(), signature);
    }

    template<scan_alignment alignment>
    scan_result find_pattern(signature_view signature, std::string_view section, module_t mod) {
        const auto data = get_section_data(mod, section);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern<alignment>(data.begin(), data.end(), signature);
    }

    template scan_result find_pattern<scan_alignment::X1>(signature_view, module_t);
    template scan_result find_pattern<scan_alignment::X1>(signature_view, std::string_view, module_t);
    template scan_result find_pattern<scan_alignment::X16>(signature_view, module_t);
    template scan_result find_pattern<scan_alignment::X16>(signature_view, std::string_view, module_t);
}

namespace hat::detail {

    template<scan_alignment alignment>
    scan_result find_pattern(const std::byte* begin, const std::byte* end, signature_view signature) {
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi1) {
            if (ext.avx512) {
                return find_pattern<scan_mode::AVX512, alignment>(begin, end, signature);
            } else if (ext.avx2) {
                return find_pattern<scan_mode::AVX2, alignment>(begin, end, signature);
            }
        }
        if (ext.sse41) {
            return find_pattern<scan_mode::SSE, alignment>(begin, end, signature);
        }
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return find_pattern<scan_mode::Single, alignment>(begin, end, signature);
    }

    template scan_result find_pattern<scan_alignment::X1>(const std::byte*, const std::byte*, signature_view);
    template scan_result find_pattern<scan_alignment::X16>(const std::byte*, const std::byte*, signature_view);
}
