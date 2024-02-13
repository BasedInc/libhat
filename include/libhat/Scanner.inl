#pragma once

namespace hat {

    template<scan_alignment alignment>
    scan_result find_pattern(const signature_view signature, const hat::process::module_t mod) {
        const auto data = hat::process::get_module_data(mod);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern<alignment>(data.begin(), data.end(), signature);
    }

    template<scan_alignment alignment>
    scan_result find_pattern(const signature_view signature, const std::string_view section, const hat::process::module_t mod) {
        const auto data = hat::process::get_section_data(mod, section);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern<alignment>(data.begin(), data.end(), signature);
    }
}
