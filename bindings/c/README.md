# libhat_c

This library serves as the official bridge between libhat's C++ implementation and other languages that support C FFI.
It currently supports most of libhat's core features, including:

- `libhat/process.hpp`
- `hat::find_pattern`

### Memory Management

Some functions may return dynamically allocated objects, either directly or through output parameters. These objects
are represented by opaque pointers, and must be manually freed to avoid memory and other resource leaks. Any pointer
returned by a libhat function that is not a `libhat_*` opaque type is assumed to have static lifetime, and is not
expected to be freed by the user. This functionality is exposed through a single function `libhat_free`, for example:

```c
const void* find_abc(const void* buf, size_t n) {
    const libhat_signature* sig;
    if (libhat_success != libhat_parse_signature("61 62 63", &sig)) {
        puts("libhat_parse_signature failed");
        return;
    }

    const void* result;
    if (libhat_success != libhat_find_pattern(sig, buf, n, &result, /* ... */)) {
        puts("libhat_find_pattern failed");
        libhat_free(sig);
        return;
    }

    libhat_free(sig);
    return result;
}
```

`libhat_free(NULL)` is entirely valid and ignored by the implementation.

### API Hardening

The libhat_c implementation is hardened, and will attempt to detect API misuse at runtime and prevent errors that could
result in memory corruption, undefined behavior, or segmentation faults. *Ideally*, bindings for other languages that
wrap the C API should provide abstractions that exclude the possibility of triggering such measures, and catch errors
before the C implementation is invoked whenever applicable. The following usage errors and their effects are as follows:

| Error                                                    | Effect                              |
|----------------------------------------------------------|-------------------------------------|
| Double free via `libhat_free`                            | `std::terminate`                    |
| Passing a non-libhat type to `libhat_free`               | `std::terminate`                    |
| Passing a non-libhat type as an opaque pointer parameter | `libhat_err_invalid_argument_type`  |
| Passing a mismatched type as an opaque pointer parameter | `libhat_err_invalid_argument_type`  |
| Passing `nullptr` for a non-nullable parameter           | `libhat_err_invalid_argument_value` |
| Passing a value not expressible by a parameter's type    | `libhat_err_invalid_argument_value` |

It should be noted that any non-null pointer arguments are still assumed to be valid, readable virtual memory addresses,
and the mitigations serve to validate that the memory pointed to is correct, not whether the pointer itself is correct.
