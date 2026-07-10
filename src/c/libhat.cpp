#include <libhat/c/libhat.h>

#include <libhat/scanner.hpp>
#include <cstring>

namespace {

    struct libhat_ffi_object {
        virtual ~libhat_ffi_object() = default;
    };

    template<typename T>
    struct libhat_ffi_wrapper : libhat_ffi_object, T {

        template<typename... Args>
        explicit libhat_ffi_wrapper(Args&&... args) : T(std::forward<Args>(args)...) {}
    };
}

struct libhat_signature final : libhat_ffi_wrapper<hat::signature> {
    using libhat_ffi_wrapper::libhat_ffi_wrapper;
};

struct libhat_module final : libhat_ffi_wrapper<hat::process::module> {
    using libhat_ffi_wrapper::libhat_ffi_wrapper;
};

static hat::scan_alignment to_cpp_align(const libhat_alignment align) {
    switch (align) {
        case libhat_alignment_x1:
            return hat::scan_alignment::X1;
        case libhat_alignment_x4:
            return hat::scan_alignment::X4;
        case libhat_alignment_x16:
            return hat::scan_alignment::X16;
    }
    exit(EXIT_FAILURE);
}

static hat::scan_hint to_cpp_hints(const libhat_hint hints) {
    return static_cast<hat::scan_hint>(hints);
}

extern "C" {

LIBHAT_API libhat_status libhat_parse_signature(const char* signatureStr, const libhat_signature** signatureOut) {
    auto result = hat::parse_signature(signatureStr);
    if (!result.has_value()) {
        *signatureOut = nullptr;
        switch (result.error()) {
            using enum hat::signature_error;
            case missing_masked_byte:  return libhat_err_sig_missing_masked_byte;
            case element_parse_error:  return libhat_err_sig_element_parse_error;
            case empty_signature:      return libhat_err_sig_empty_signature;
            case expected_wildcard:    return libhat_err_sig_expected_wildcard;
            case invalid_token_length: return libhat_err_sig_invalid_token_length;
        }
        return libhat_err_unknown;
    }
    *signatureOut = new libhat_signature{std::move(result).value()};
    return libhat_success;
}

LIBHAT_API libhat_status libhat_create_signature(
    const char*              bytes,
    const char*              mask,
    const size_t             size,
    const libhat_signature** signatureOut
) {
    hat::signature signature{};
    bool containsByte = false;
    signature.reserve(size);
    for (size_t i{}; i < size; i++) {
        containsByte |= signature.emplace_back(
            static_cast<std::byte>(bytes[i]),
            static_cast<std::byte>(mask[i])
        ).all();
    }
    if (!containsByte) {
        return libhat_err_sig_missing_masked_byte;
    }
    *signatureOut = new libhat_signature{std::move(signature)};
    return libhat_success;
}

LIBHAT_API const void* libhat_find_pattern(
    const libhat_signature* signature,
    const void*             buffer,
    const size_t            size,
    const libhat_alignment  align,
    const libhat_hint       hints
) {
    const auto begin = static_cast<const std::byte*>(buffer);
    const auto end = static_cast<const std::byte*>(buffer) + size;
    const auto result = hat::find_pattern(begin, end, *signature, to_cpp_align(align), to_cpp_hints(hints));
    return result.has_result() ? result.get() : nullptr;
}

LIBHAT_API const void* libhat_find_pattern_mod(
    const libhat_signature* signature,
    const libhat_module*    module,
    const char*             section,
    const libhat_alignment  align,
    const libhat_hint       hints
) {
    const auto result = hat::find_pattern(*signature, section, *module, to_cpp_align(align), to_cpp_hints(hints));
    return result.has_result() ? result.get() : nullptr;
}

LIBHAT_API uintptr_t libhat_module_address(const libhat_module* module) {
    return module->address();
}

LIBHAT_API libhat_span libhat_module_get_data(const libhat_module* module) {
    const auto data = module->get_module_data();
    return {data.data(), data.size()};
}

LIBHAT_API libhat_span libhat_module_get_executable_data(const libhat_module* module) {
    const auto data = module->get_executable_data();
    return {data.data(), data.size()};
}

LIBHAT_API libhat_span libhat_module_get_section_data(const libhat_module* module, const char* name) {
    const auto data = module->get_section_data(name);
    return {data.data(), data.size()};
}

LIBHAT_API void libhat_module_for_each_section(const libhat_module* module, const libhat_for_each_section_cb callback, void* user_data) {
    std::string buffer;
    module->for_each_section([=, &buffer](auto name, auto data, auto prot) {
        buffer.assign(name);
        return callback(buffer.c_str(), {data.data(), data.size()}, static_cast<libhat_protection>(prot), user_data);
    });
}

LIBHAT_API void libhat_module_for_each_segment(const libhat_module* module, const libhat_for_each_segment_cb callback, void* user_data) {
    module->for_each_segment([=](auto data, auto prot) {
        return callback({data.data(), data.size()}, static_cast<libhat_protection>(prot), user_data);
    });
}

LIBHAT_API bool libhat_is_readable(const void* data, size_t size) {
    return hat::process::is_readable({static_cast<const std::byte*>(data), size});
}

LIBHAT_API bool libhat_is_writable(const void* data, size_t size) {
    return hat::process::is_writable({static_cast<const std::byte*>(data), size});
}

LIBHAT_API bool libhat_is_executable(const void* data, size_t size) {
    return hat::process::is_executable({static_cast<const std::byte*>(data), size});
}

LIBHAT_API const libhat_module* libhat_get_process_module() {
    return new libhat_module{hat::process::get_process_module()};
}

LIBHAT_API const libhat_module* libhat_get_module(const char* name) {
    if (!name) {
        return libhat_get_process_module();
    }
    if (auto mod = hat::process::get_module(name); mod.has_value()) {
        return new libhat_module{std::move(mod).value()};
    }
    return nullptr;
}

LIBHAT_API const libhat_module* libhat_module_at(const void* address) {
    if (auto mod = hat::process::module_at(address); mod.has_value()) {
        return new libhat_module{std::move(mod).value()};
    }
    return nullptr;
}

LIBHAT_API void libhat_free(const void* object) {
    delete static_cast<const libhat_ffi_object*>(object);
}

} // extern "C"
