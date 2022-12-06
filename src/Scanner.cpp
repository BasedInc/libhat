#include <libhat/Scanner.hpp>

#include <map>
#include <execution>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>
#include <libhat/ScanMode.hpp>

namespace hat {

    using namespace hat::process;

    template<>
    scan_result find_vtable<compiler_type::MSVC>(const std::string& className, module_t module) {
        // Tracing cross-references
        // Type Descriptor => Object Locator => VTable
        auto sig = string_to_signature(".?AV" + className + "@@");

        // TODO: Have a better solution for this
        // 3rd character may be 'V' for classes and 'U' for structs
        sig[3] = {};

        auto typeDesc = *find_pattern(sig, ".data", module);
        if (!typeDesc) {
            return nullptr;
        }
        // 0x10 is the offset from the type descriptor name to the type descriptor header
        typeDesc -= 2 * sizeof(void*);

        // The actual xref refers to an offset from the base module
        const auto loffset = static_cast<uint32_t>(typeDesc - reinterpret_cast<std::byte*>(module));
        auto locator = object_to_signature(loffset);
        // FIXME: These appear to be the values just for basic classes with single inheritance. We should be using a
        //        different method to differentiate the object locator from the base class descriptor.
        #ifdef LIBHAT_X86_64
        locator.insert(locator.begin(), {
            std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // signature
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // offset
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}  // constructor displacement offset
        });
        #else
        locator.insert(locator.begin(), {
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // signature
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, // offset
            std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}  // constructor displacement offset
        });
        #endif
        const auto objectLocator = *find_pattern(locator, ".rdata", module);
        if (!objectLocator) {
            return nullptr;
        }

        const auto vtable = *find_pattern(object_to_signature(objectLocator), ".data", module);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }

    template<>
    scan_result find_vtable<compiler_type::MinGW>(const std::string& className, module_t module) {
        // Tracing cross-references
        // Type Descriptor Name => Type Info => VTable
        const auto sig = string_to_signature(std::to_string(className.size()) + className + "\0");
        const auto typeName = *find_pattern(sig, ".rdata", module);
        if (!typeName) {
            return nullptr;
        }
        auto typeInfo = *find_pattern(object_to_signature(typeName), ".rdata", module);
        if (!typeInfo) {
            return nullptr;
        }
        // A single pointer is the offset from the type name pointer to the start of the type info
        typeInfo -= sizeof(void*);

        const auto vtable = *find_pattern(object_to_signature(typeInfo), ".rdata", module);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }

    scan_result find_pattern(signature_view signature, module_t module) {
        const auto data = get_module_data(module);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern(std::to_address(data.begin()), std::to_address(data.end()), signature);
    }

    scan_result find_pattern(signature_view signature, std::string_view section, module_t module) {
        const auto data = get_section_data(module, section);
        if (data.empty()) {
            return nullptr;
        }
        return find_pattern(std::to_address(data.begin()), std::to_address(data.end()), signature);
    }

    template<>
    [[deprecated]] scan_result find_pattern<scan_mode::Search>(std::byte* begin, std::byte* end, signature_view signature) {
        auto it = std::search(
            begin, end,
            signature.begin(), signature.end(),
            [](auto byte, auto opt) {
                return !opt.has_value() || *opt == byte;
            });
        return it != end ? it : nullptr;
    }

    template<>
    scan_result find_pattern<scan_mode::FastFirst>(std::byte* begin, std::byte* end, signature_view signature) {
        const auto firstByte = *signature[0];
        const auto scanEnd = end - signature.size() + 1;

        for (auto i = begin; i != scanEnd; i++) {
            // Use std::find to efficiently find the first byte
            i = std::find(std::execution::unseq, i, scanEnd, firstByte);
            if (i == scanEnd) {
                break;
            }
            // Compare everything after the first byte
            auto match = std::equal(signature.begin() + 1, signature.end(), i + 1, [](auto opt, auto byte) {
                return !opt.has_value() || *opt == byte;
            });
            if (match) {
                return i;
            }
        }
        return nullptr;
    }

    scan_result find_pattern(std::byte* begin, std::byte* end, signature_view signature) {
        const auto size = signature.size();
#if defined(LIBHAT_X86)
        const auto& ext = get_system().extensions;
        if (ext.bmi1 && ext.popcnt) {
            if (size <= 65 && ext.avx512) {
                return find_pattern<scan_mode::AVX512>(begin, end, signature);
            } else if (size <= 33 && ext.avx2) {
                return find_pattern<scan_mode::AVX2>(begin, end, signature);
            }
        }
#elif defined(LIBHAT_ARM)
        if (size <= 17) {
            return find_pattern<scan_mode::Neon>(begin, end, signature);
        }
#endif
        // If none of the vectorized implementations are available/supported, then fallback to scanning per-byte
        return find_pattern<scan_mode::FastFirst>(begin, end, signature);
    }
}
