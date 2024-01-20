# libhat
A modern, high-performance library for C++20 designed around game hacking

## Feature overview
- Windows x86/x64 support
- Vectorized scanning for byte patterns
- RAII memory protector
- Convenience wrappers over OS APIs
- C bindings

## Quick start
### Pattern scanning
```cpp
#include <libhat.hpp>

// Convert a pattern's string representation to an array of bytes at compile time
static constexpr auto pattern = hat::compile_signature<"E8 ? ? ? ? 1A 2B 3C 4D">();

// ...or convert it at runtime (returns hat::result)
auto runtime_pattern = hat::parse_signature("E8 ? ? ? ? 1A 2B 3C 4D");

// Scan for this pattern using your CPU's vectorization features
hat::scan_result result = hat::find_pattern(pattern);

// Get the address pointed at by the pattern
const std::byte* address = result.get();

// Jump to an RIP relative address at a given offset
const std::byte* relative_address = result.rel(1);
```

### Accessing offsets
```cpp
#include <libhat.hpp>

// Get the value at a given offset inside of an object
std::string example = "test";

// Here, I am accessing the value at offset 0x10 of a std::string, which is a size_t
// If the object is passed as a reference, the return value must be const
const size_t& size = hat::member_at<size_t>(&std::as_const(example), 0x10);

// Additionally, you can also do this:
size_t& size = hat::member_at<size_t>(example, 0x10);
```
