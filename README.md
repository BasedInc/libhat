# libhat
A modern, high-performance library for C++20 designed around game hacking

## Feature overview
- Windows x86/x64 support
- Vectorized scanning for byte patterns
- RAII memory protector
- Convenience wrappers over OS APIs
- Language bindings (C, C#, etc.)

## Quick start
### Pattern scanning
```cpp
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
// An example struct and it's member offsets
struct S {
    uint32_t a{}; // 0x0
    uint32_t b{}; // 0x4
    uint32_t c{}; // 0x8
    uint32_t d{}; // 0xC
};

S s;

// Obtain a mutable reference to 's.b' via it's offset
uint32_t& b = hat::member_at<uint32_t>(&s, 0x4);

// If the provided pointer is const, the returned reference is const
const uint32_t& b = hat::member_at<uint32_t>(&std::as_const(s), 0x4);
```
