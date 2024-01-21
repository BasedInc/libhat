#pragma once

#if defined(LIBHAT_BUILD_SHARED_LIB)
    #define LIBHAT_API __declspec(dllexport)
#elif defined(LIBHAT_USE_SHARED_LIB)
    #define LIBHAT_API __declspec(dllimport)
#else
    #define LIBHAT_API
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum libhat_status_t {
    libhat_success,         // The operation was successful
    libhat_err_unknown,
    libhat_err_sig_invalid, // The signature contained an invalid byte
    libhat_err_sig_empty,   // The signature is empty
    libhat_err_sig_nobyte,  // The signature did not contain a present byte, only wildcards
} libhat_status_t;

typedef enum scan_alignment {
    scan_alignment_x1,
    scan_alignment_x16,
} scan_alignment_t;

typedef struct signature {
    void* data;
    size_t count;
} signature_t;

LIBHAT_API libhat_status_t libhat_parse_signature(
    const char*   signatureStr,
    signature_t** signatureOut
);

LIBHAT_API libhat_status_t libhat_create_signature(
    const char*   bytes,
    const char*   mask,
    size_t        size,
    signature_t** signatureOut
);

LIBHAT_API const void* libhat_find_pattern(
    const signature_t*  signature,
    const void*         buffer,
    size_t              size,
    scan_alignment      align
);

LIBHAT_API const void* libhat_find_pattern_mod(
    const signature_t*  signature,
    const void*         module,
    const char*         section,
    scan_alignment      align
);

LIBHAT_API const void* libhat_get_module(const char* name);

LIBHAT_API void libhat_free(void* mem);

#ifdef __cplusplus
}
#endif