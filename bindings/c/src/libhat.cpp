#include <libhat/c/libhat.h>

#include <libhat/scanner.hpp>
#include <cstring>

namespace {

    constexpr uint32_t object_magic = 0x2C360B8A;

    struct tombstone_type {
        static constexpr uint32_t type_id = 0xF27EC9D1;
    };

    template<typename T>
    consteval uint32_t type_id() {
        return T::type_id;
    }

    struct libhat_ffi_object {
    protected:
        uint32_t magic;
        mutable std::atomic<uint32_t> type;
        void (*destroy)(const libhat_ffi_object&);

        template<typename Derived>
        explicit libhat_ffi_object(std::type_identity<Derived>) :
            magic(object_magic),
            type(type_id<Derived>()),
            destroy([](const libhat_ffi_object& object) {
                delete static_cast<const Derived*>(&object);
            }) {}

        friend void ::libhat_free(const void* object);

    public:
        template<typename T>
        [[nodiscard]] bool is_type() const {
            return this->magic == object_magic && this->type == type_id<T>();
        }
    };

    template<typename Derived, typename T>
    struct libhat_ffi_wrapper : libhat_ffi_object, T {

        template<typename... Args>
        explicit libhat_ffi_wrapper(Args&&... args) :
            libhat_ffi_object(std::type_identity<Derived>{}),
            T(std::forward<Args>(args)...) {}
    };

    template<typename T>
    [[nodiscard]] bool check_type(const T* t) {
        return static_cast<const libhat_ffi_object*>(t)->is_type<T>();
    }
}

struct libhat_signature final : libhat_ffi_wrapper<libhat_signature, hat::signature> {
    using libhat_ffi_wrapper::libhat_ffi_wrapper;
    static constexpr uint32_t type_id = 0xFD19C2B3;
};

struct libhat_module final : libhat_ffi_wrapper<libhat_module, hat::process::module> {
    using libhat_ffi_wrapper::libhat_ffi_wrapper;
    static constexpr uint32_t type_id = 0xBAA5FEEC;
};

static std::optional<hat::scan_alignment> to_cpp_align(const libhat_alignment align) {
    switch (align) {
        case libhat_alignment_x1:
            return hat::scan_alignment::X1;
        case libhat_alignment_x4:
            return hat::scan_alignment::X4;
        case libhat_alignment_x16:
            return hat::scan_alignment::X16;
    }
    return std::nullopt;
}

static hat::scan_hint to_cpp_hints(const libhat_hint hints) {
    return static_cast<hat::scan_hint>(hints);
}

extern "C" {

LIBHAT_API const char* libhat_get_version() {
    return LIBHAT_VERSION;
}

LIBHAT_API int libhat_get_version_num() {
    return (LIBHAT_VERSION_MAJOR << 16) | (LIBHAT_VERSION_MINOR << 8) | LIBHAT_VERSION_PATCH;
}

LIBHAT_API const char* libhat_status_to_string(const libhat_status status) {
#define STATUS_CASE(x) case x: return #x
    switch (status) {
        STATUS_CASE(libhat_success);
        STATUS_CASE(libhat_err_unknown);
        STATUS_CASE(libhat_err_sig_missing_masked_byte);
        STATUS_CASE(libhat_err_sig_element_parse_error);
        STATUS_CASE(libhat_err_sig_empty_signature);
        STATUS_CASE(libhat_err_sig_expected_wildcard);
        STATUS_CASE(libhat_err_sig_invalid_token_length);
        STATUS_CASE(libhat_err_invalid_argument_value);
        STATUS_CASE(libhat_err_invalid_argument_type);
    }
#undef STATUS_CASE
    return "invalid value for libhat_status";
}

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
    if (!signatureOut) {
        return libhat_err_invalid_argument_value;
    }
    if (size && (!bytes || !mask)) {
        return libhat_err_invalid_argument_value;
    }
    if (!size) {
        return libhat_err_sig_empty_signature;
    }

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

LIBHAT_API libhat_status libhat_find_pattern(
    const libhat_signature* signature,
    const void*             buffer,
    const size_t            size,
    const void**            resultOut,
    const libhat_alignment  align,
    const libhat_hint       hints
) {
    if (!signature || (!buffer && size) || !resultOut) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(signature)) {
        return libhat_err_invalid_argument_type;
    }

    const auto cpp_align = to_cpp_align(align);
    if (!cpp_align) {
        return libhat_err_invalid_argument_value;
    }

    const auto begin = static_cast<const std::byte*>(buffer);
    const auto end = static_cast<const std::byte*>(buffer) + size;
    const auto result = hat::find_pattern(begin, end, *signature, *cpp_align, to_cpp_hints(hints));
    *resultOut = result.has_result() ? result.get() : nullptr;
    return libhat_success;
}

LIBHAT_API libhat_status libhat_find_pattern_mod(
    const libhat_signature* signature,
    const libhat_module*    module,
    const char*             section,
    const void**            resultOut,
    const libhat_alignment  align,
    const libhat_hint       hints
) {
    if (!signature || !module || !section || !resultOut) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(signature) || !check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    const auto cpp_align = to_cpp_align(align);
    if (!cpp_align) {
        return libhat_err_invalid_argument_value;
    }

    const auto result = hat::find_pattern(*signature, section, *module, *cpp_align, to_cpp_hints(hints));
    *resultOut = result.has_result() ? result.get() : nullptr;
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_address(const libhat_module* module, uintptr_t* out) {
    if (!module || !out) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    *out = module->address();
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_get_symbol(const libhat_module* module, const char* name, uintptr_t* out) {
    if (!module || !name || !out) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    *out = module->get_symbol(name);
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_get_data(const libhat_module* module, libhat_span* out) {
    if (!module || !out) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    const auto data = module->get_module_data();
    *out = {data.data(), data.size()};
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_get_executable_data(const libhat_module* module, libhat_span* out) {
    if (!module || !out) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    const auto data = module->get_executable_data();
    *out = {data.data(), data.size()};
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_get_section_data(const libhat_module* module, const char* name, libhat_span* out) {
    if (!module || !name || !out) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    const auto data = module->get_section_data(name);
    *out = {data.data(), data.size()};
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_for_each_section(const libhat_module* module, const libhat_for_each_section_cb callback, void* user_data) {
    if (!module || !callback) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    std::string buffer;
    module->for_each_section([=, &buffer](auto name, auto data, auto prot) {
        buffer.assign(name);
        return callback(buffer.c_str(), {data.data(), data.size()}, static_cast<libhat_protection>(prot), user_data);
    });
    return libhat_success;
}

LIBHAT_API libhat_status libhat_module_for_each_segment(const libhat_module* module, const libhat_for_each_segment_cb callback, void* user_data) {
    if (!module || !callback) {
        return libhat_err_invalid_argument_value;
    }

    if (!check_type(module)) {
        return libhat_err_invalid_argument_type;
    }

    module->for_each_segment([=](auto data, auto prot) {
        return callback({data.data(), data.size()}, static_cast<libhat_protection>(prot), user_data);
    });
    return libhat_success;
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
    if (!object) {
        return;
    }

    const auto& p = *static_cast<const libhat_ffi_object*>(object);
    if (p.magic != object_magic) {
        std::terminate();
    }

    constexpr auto tombstone = type_id<tombstone_type>();
    if (p.type.exchange(tombstone, std::memory_order_relaxed) == tombstone) {
        // Object not within its lifetime, already deleted. This could be indicative of a larger
        // issue in user code, so it's best to not just ignore it. Not going to make this return
        // a status code, just terminate for now.
        std::terminate();
    }

    p.destroy(p);
}

} // extern "C"
