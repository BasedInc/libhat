#include <libhat/c/libhat.h>

#include <libhat/Scanner.hpp>

extern "C" {

LIBHAT_API libhat_status_t libhat_parse_signature(const char* signatureStr, signature_t** signatureOut) {
    auto result = hat::parse_signature(signatureStr);
    if (!result.has_value()) {
        *signatureOut = nullptr;
        switch (result.error()) {
            using enum hat::signature_parse_error;
            case missing_byte:    return libhat_err_sig_nobyte;
            case parse_error:     return libhat_err_sig_invalid;
            case empty_signature: return libhat_err_sig_empty;
        }
        return libhat_err_unknown;
    }

    const auto bytes = std::as_bytes(std::span{result.value()});

    auto* mem = malloc(sizeof(signature_t) + bytes.size());
    auto* sig = static_cast<signature_t*>(mem);
    sig->data = static_cast<std::byte*>(mem) + sizeof(signature_t);
    sig->count = result.value().size();

    std::memcpy(sig->data, bytes.data(), bytes.size());

    *signatureOut = sig;

    return libhat_success;
}

LIBHAT_API const void* libhat_find_pattern(
    const signature_t*   signature,
    const void*          buffer,
    const size_t         size,
    const scan_alignment align
) {
    const hat::signature_view view{
        static_cast<hat::signature_element*>(signature->data),
        signature->count
    };

    const auto find_pattern = [=]<hat::scan_alignment A>() {
        const auto begin = static_cast<const std::byte*>(buffer);
        const auto end = static_cast<const std::byte*>(buffer) + size;
        const auto result = hat::find_pattern(begin, end, view);
        return result.has_result() ? result.get() : nullptr;
    };

    switch (align) {
        case scan_alignment_x1:
            return find_pattern.operator()<hat::scan_alignment::X1>();
        case scan_alignment_x16:
            return find_pattern.operator()<hat::scan_alignment::X16>();
    }
    exit(EXIT_FAILURE);
}

LIBHAT_API const void* libhat_find_pattern_mod(
    const signature_t*   signature,
    const void*          module,
    const char*          section,
    const scan_alignment align
) {
    const hat::signature_view view{
        static_cast<hat::signature_element*>(signature->data),
        signature->count
    };

    const auto find_pattern = [=]<hat::scan_alignment A>() {
        const auto mod = hat::process::module_at(reinterpret_cast<uintptr_t>(module));
        const auto result = hat::find_pattern(view, section, mod);
        return result.has_result() ? result.get() : nullptr;
    };

    switch (align) {
        case scan_alignment_x1:
            return find_pattern.operator()<hat::scan_alignment::X1>();
        case scan_alignment_x16:
            return find_pattern.operator()<hat::scan_alignment::X16>();
    }
    exit(EXIT_FAILURE);
}

LIBHAT_API void libhat_free(void* mem) {
    free(mem);
}

} // extern "C"