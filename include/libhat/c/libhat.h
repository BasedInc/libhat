#pragma once

#include "../platform.h"

#if defined(LIBHAT_WINDOWS)
    #if defined(LIBHAT_BUILD_SHARED_LIB)
        #define LIBHAT_API __declspec(dllexport)
    #elif defined(LIBHAT_USE_SHARED_LIB)
        #define LIBHAT_API __declspec(dllimport)
    #else
        #define LIBHAT_API
    #endif
#elif defined(LIBHAT_UNIX)
    #if defined(LIBHAT_BUILD_SHARED_LIB)
        #define LIBHAT_API __attribute__((visibility("default")))
    #else
        #define LIBHAT_API
    #endif
#else
    #define LIBHAT_API
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum libhat_status {
    libhat_success,         // The operation was successful
    libhat_err_unknown,
    libhat_err_sig_missing_masked_byte,
    libhat_err_sig_element_parse_error,
    libhat_err_sig_empty_signature,
    libhat_err_sig_expected_wildcard,
    libhat_err_sig_invalid_token_length,
    libhat_err_invalid_argument_value,
    libhat_err_invalid_argument_type,
} libhat_status;

typedef enum libhat_alignment {
    libhat_alignment_x1 = 1,
    libhat_alignment_x4 = 4,
    libhat_alignment_x16 = 16,
} libhat_alignment;

typedef enum libhat_hint {
    libhat_hint_none    = 0,
    libhat_hint_x86_64  = 1 << 0,
    libhat_hint_pair0   = 1 << 1,
    libhat_hint_aarch64 = 1 << 2,
} libhat_hint;

typedef enum libhat_protection {
    libhat_protection_none    = 0b000,
    libhat_protection_read    = 0b001,
    libhat_protection_write   = 0b010,
    libhat_protection_execute = 0b100,
} libhat_protection;

typedef struct libhat_signature libhat_signature;
typedef struct libhat_module libhat_module;

typedef struct libhat_span {
    const void* data;
    size_t      size;
} libhat_span;

typedef bool(*libhat_for_each_section_cb)(const char* name, libhat_span data, libhat_protection prot, void* user_data);
typedef bool(*libhat_for_each_segment_cb)(libhat_span data, libhat_protection prot, void* user_data);

/// Returns the libhat version as a string, in the form @code"major.minor.patch"@endcode
LIBHAT_API const char* libhat_get_version();

/// Returns the libhat version as a number, in the form @code(major << 16) | (minor << 8) | (patch)@endcode
LIBHAT_API int libhat_get_version_num();

LIBHAT_API const char* libhat_status_to_string(libhat_status status);

LIBHAT_API libhat_status libhat_parse_signature(
    const char*              signatureStr,
    const libhat_signature** signatureOut
);

LIBHAT_API libhat_status libhat_create_signature(
    const char*              bytes,
    const char*              mask,
    size_t                   size,
    const libhat_signature** signatureOut
);

LIBHAT_API libhat_status libhat_find_pattern(
    const libhat_signature* signature,
    const void*             buffer,
    size_t                  size,
    const void**            resultOut,
    libhat_alignment        align,
    libhat_hint             hints
);

LIBHAT_API libhat_status libhat_find_pattern_mod(
    const libhat_signature* signature,
    const libhat_module*    module,
    const char*             section,
    const void**            resultOut,
    libhat_alignment        align,
    libhat_hint             hints
);

LIBHAT_API libhat_status libhat_module_address(const libhat_module* module, uintptr_t* out);

LIBHAT_API libhat_status libhat_module_get_data(const libhat_module* module, libhat_span* out);

LIBHAT_API libhat_status libhat_module_get_executable_data(const libhat_module* module, libhat_span* out);

LIBHAT_API libhat_status libhat_module_get_section_data(const libhat_module* module, const char* name, libhat_span* out);

LIBHAT_API libhat_status libhat_module_for_each_section(
    const libhat_module*       module,
    libhat_for_each_section_cb callback,
    void*                      user_data
);

LIBHAT_API libhat_status libhat_module_for_each_segment(
    const libhat_module*       module,
    libhat_for_each_segment_cb callback,
    void*                      user_data
);

LIBHAT_API bool libhat_is_readable(const void* data, size_t size);

LIBHAT_API bool libhat_is_writable(const void* data, size_t size);

LIBHAT_API bool libhat_is_executable(const void* data, size_t size);

LIBHAT_API const libhat_module* libhat_get_process_module();

LIBHAT_API const libhat_module* libhat_get_module(const char* name);

LIBHAT_API const libhat_module* libhat_module_at(const void* address);

LIBHAT_API void libhat_free(const void* object);

#ifdef __cplusplus
}
#endif
