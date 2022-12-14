#include <libhat/Scanner.hpp>

#include <map>

#include <libhat/Defines.hpp>
#include <libhat/System.hpp>

namespace hat {

    using namespace hat::process;

    template<>
    scan_result find_vtable<compiler_type::MSVC>(const std::string& className, module_t mod) {
        // Tracing cross-references
        // Type Descriptor => Object Locator => VTable
        auto sig = string_to_signature(".?AV" + className + "@@");

        // TODO: Have a better solution for this
        // 3rd character may be 'V' for classes and 'U' for structs
        sig[3] = {};

        auto typeDesc = *find_pattern(sig, ".data", mod);
        if (!typeDesc) {
            return nullptr;
        }
        // 0x10 is the offset from the type descriptor name to the type descriptor header
        typeDesc -= 2 * sizeof(void*);

        // The actual xref refers to an offset from the base module
        const auto loffset = static_cast<uint32_t>(typeDesc - reinterpret_cast<std::byte*>(mod));
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
        const auto objectLocator = *find_pattern(locator, ".rdata", mod);
        if (!objectLocator) {
            return nullptr;
        }

        const auto vtable = *find_pattern(object_to_signature(objectLocator), ".data", mod);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }

    template<>
    scan_result find_vtable<compiler_type::MinGW>(const std::string& className, module_t mod) {
        // Tracing cross-references
        // Type Descriptor Name => Type Info => VTable
        const auto sig = string_to_signature(std::to_string(className.size()) + className + "\0");
        const auto typeName = *find_pattern(sig, ".rdata", mod);
        if (!typeName) {
            return nullptr;
        }
        auto typeInfo = *find_pattern(object_to_signature(typeName), ".rdata", mod);
        if (!typeInfo) {
            return nullptr;
        }
        // A single pointer is the offset from the type name pointer to the start of the type info
        typeInfo -= sizeof(void*);

        const auto vtable = *find_pattern(object_to_signature(typeInfo), ".rdata", mod);
        return vtable ? vtable + sizeof(void*) : nullptr;
    }

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
