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

#include <stddef.h>

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
} libhat_status;

typedef enum libhat_alignment {
    libhat_alignment_x1 = 1,
    libhat_alignment_x4 = 4,
    libhat_alignment_x16 = 16,
} libhat_alignment;

typedef struct libhat_signature libhat_signature;
typedef struct libhat_module libhat_module;

LIBHAT_API libhat_status libhat_parse_signature(
    const char*        signatureStr,
    libhat_signature** signatureOut
);

LIBHAT_API libhat_status libhat_create_signature(
    const char*        bytes,
    const char*        mask,
    size_t             size,
    libhat_signature** signatureOut
);

LIBHAT_API const void* libhat_find_pattern(
    const libhat_signature* signature,
    const void*             buffer,
    size_t                  size,
    libhat_alignment        align
);

LIBHAT_API const void* libhat_find_pattern_mod(
    const libhat_signature* signature,
    const libhat_module*    module,
    const char*             section,
    libhat_alignment        align
);

LIBHAT_API const libhat_module* libhat_get_module(const char* name);

LIBHAT_API void libhat_free(void* mem);

#ifdef __cplusplus
}
#endif
