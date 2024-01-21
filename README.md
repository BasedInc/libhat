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
// Parse a pattern's string representation to an array of bytes at compile time
constexpr hat::fixed_signature pattern = hat::compile_signature<"48 8D 05 ? ? ? ? E8">();

// ...or parse it at runtime
using parsed_t = hat::result<hat::signature, hat::signature_parse_error>;
parsed_t runtime_pattern = hat::parse_signature("48 8D 05 ? ? ? ? E8");

// Scan for this pattern using your CPU's vectorization features
auto begin = /* a contiguous iterator over std::byte */;
auto end = /* ... */;
hat::scan_result result = hat::find_pattern(begin, end, pattern);

// Scan a section in the process's base module
hat::scan_result result = hat::find_pattern(pattern, ".text");

// Or another module loaded into the process
hat::process::module_t ntdll = hat::process::get_module("ntdll.dll");
hat::scan_result result = hat::find_pattern(pattern, ntdll, ".text");

// Get the address pointed at by the pattern
const std::byte* address = result.get();

// Resolve an RIP relative address at a given offset
// 
//   | signature matches here
//   |        | relative address located at +3
//   v        v
//   48 8D 05 BE 53 23 01    lea  rax, [rip+0x12353be]
//
const std::byte* relative_address = result.rel(3);
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

### Writing to protected memory
```cpp
uintptr_t* vftable = ...;       // Pointer to a virtual function table in read-only data
size_t target_func_index = ...; // Index to an interesting function

// Use memory_protector to enable write protections
hat::memory_protector prot{
    (uintptr_t) &vftable[target_func_index],        // a pointer to the target memory
    sizeof(uintptr_t),                              // the size of the memory block
    hat::protection::Read | hat::protection::Write  // the new protection flags
};

// Overwrite function table entry to redirect to a custom callback
vftable[target_func_index] = (uintptr_t) my_callback;

// On scope exit, original protections will be restored
prot.~memory_protector(); // compiler generated

```
