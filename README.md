# libhat
A modern, high-performance library for C++20 designed around game hacking

## Feature overview
- Windows x86/x64 support
- Partial Linux and macOS support
- Vectorized scanning for byte patterns
- RAII memory protector
- Convenience wrappers over OS APIs
- Language bindings (C, C#, etc.)

## Versioning
This project adheres to [semantic versioning](https://semver.org/spec/v2.0.0.html). Any declaration that
is within a `detail` or `experimental` namespace is not considered part of the public API, and usage
may break at any time without the MAJOR version number being incremented.

## Benchmarks
The table below compares the single threaded throughput in bytes/s (real time) between
libhat and [two other](test/benchmark/vendor) commonly used implementations for pattern
scanning. The input buffers were randomly generated using a fixed seed, and the pattern
scanned does not contain any match in the buffer. The benchmark was run on a system with
an i7-9700K (which supports libhat's [AVX2](src/arch/x86/AVX2.cpp) scanner implementation).
The full source code is available [here](test/benchmark/Compare.cpp).
```
---------------------------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations bytes_per_second
---------------------------------------------------------------------------------------
BM_Throughput_Libhat/4MiB       131578 ns        48967 ns        21379      29.6876Gi/s
BM_Throughput_Libhat/16MiB      813977 ns       413524 ns         3514      19.1959Gi/s
BM_Throughput_Libhat/128MiB    6910936 ns      3993486 ns          403      18.0873Gi/s
BM_Throughput_Libhat/256MiB   13959379 ns      8121906 ns          202      17.9091Gi/s

BM_Throughput_UC1/4MiB         4739731 ns      2776015 ns          591       843.93Mi/s
BM_Throughput_UC1/16MiB       19011485 ns     10841837 ns          147      841.597Mi/s
BM_Throughput_UC1/128MiB     152277511 ns     82465278 ns           18      840.571Mi/s
BM_Throughput_UC1/256MiB     304964544 ns    180555556 ns            9      839.442Mi/s

BM_Throughput_UC2/4MiB         9633499 ns      4617698 ns          291      415.218Mi/s
BM_Throughput_UC2/16MiB       38507193 ns     22474315 ns           73      415.507Mi/s
BM_Throughput_UC2/128MiB     307989100 ns    164930556 ns            9      415.599Mi/s
BM_Throughput_UC2/256MiB     616449240 ns    331250000 ns            5      415.282Mi/s
```

## Platforms

Below is a summary of the support of libhat OS APIs on various platforms:

|                                | Windows | Linux | macOS |
|--------------------------------|:-------:|:-----:|:-----:|
| `hat::get_system`              |    ✔    |   ✔   |   ✔   |
| `hat::memory_protector`        |    ✔    |   ✔   |       |
| `hp::get_process_module`       |    ✔    |   ✔   |       |
| `hp::get_module`               |    ✔    |   ✔   |       |
| `hp::module_at`                |    ✔    |       |       |
| `hp::is_readable`              |    ✔    |   ✔   |       |
| `hp::is_writable`              |    ✔    |   ✔   |       |
| `hp::is_executable`            |    ✔    |   ✔   |       |
| `hp::module::get_module_data`  |    ✔    |       |       |
| `hp::module::get_section_data` |    ✔    |       |       |
| `hp::module::for_each_segment` |    ✔    |   ✔   |       |

## Quick start
### Pattern scanning
```cpp
#include <libhat/scanner.hpp>

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
std::optional<hat::process::module> ntdll = hat::process::get_module("ntdll.dll");
assert(ntdll.has_value());
hat::scan_result result = hat::find_pattern(pattern, *ntdll, ".text");

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
#include <libhat/access.hpp>

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
#include <libhat/memory_protector.hpp>

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
