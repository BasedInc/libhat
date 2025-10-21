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
scanned does not contain any match in the buffer. The benchmark was compiled on Windows
with `clang-cl` 21.1.1, using the MSVC 14.44.35207 toolchain and the default release mode
flags (`/GR /EHsc /MD /O2 /Ob2`). The benchmark was run on a system with an i7-14700K
(supporting [AVX2](src/arch/x86/AVX2.cpp)) and 64GB (4x16GB) DDR5 6000 MT/s (30-38-38-96).
The full source code is available [here](test/benchmark/Compare.cpp).
```
---------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations bytes_per_second
---------------------------------------------------------------------------------------------------
BM_Throughput_libhat/4MiB                    67686 ns        67816 ns        82254      57.7110Gi/s
BM_Throughput_libhat/16MiB                  319801 ns       319558 ns        18287      48.8585Gi/s
BM_Throughput_libhat/128MiB                5325733 ns      5282315 ns         1056      23.4709Gi/s
BM_Throughput_libhat/256MiB               10921878 ns     10814951 ns          510      22.8898Gi/s

BM_Throughput_std_search/4MiB              1364050 ns      1361672 ns         4108      2.86372Gi/s
BM_Throughput_std_search/16MiB             5470025 ns      5458783 ns         1019      2.85648Gi/s
BM_Throughput_std_search/128MiB           43622456 ns     43483527 ns          129      2.86550Gi/s
BM_Throughput_std_search/256MiB           88093320 ns     87158203 ns           64      2.83790Gi/s

BM_Throughput_std_find_std_equal/4MiB       178567 ns       178586 ns        31410      21.8755Gi/s
BM_Throughput_std_find_std_equal/16MiB      806394 ns       805228 ns         7005      19.3764Gi/s
BM_Throughput_std_find_std_equal/128MiB    8944718 ns      8953652 ns          623      13.9747Gi/s
BM_Throughput_std_find_std_equal/256MiB   18092713 ns     18102751 ns          309      13.8177Gi/s

BM_Throughput_UC1/4MiB                     1727027 ns      1721236 ns         3268      2.26183Gi/s
BM_Throughput_UC1/16MiB                    6878188 ns      6849054 ns          819      2.27167Gi/s
BM_Throughput_UC1/128MiB                  55181849 ns     55300245 ns          102      2.26524Gi/s
BM_Throughput_UC1/256MiB                 110209374 ns    110000000 ns           50      2.26841Gi/s

BM_Throughput_UC2/4MiB                     4011942 ns      4001524 ns         1394      997.023Mi/s
BM_Throughput_UC2/16MiB                   16136510 ns     16166908 ns          346      991.540Mi/s
BM_Throughput_UC2/128MiB                 130954740 ns    130087209 ns           43      977.437Mi/s
BM_Throughput_UC2/256MiB                 261157833 ns    261160714 ns           21      980.250Mi/s
```

## Platforms

Below is a summary of the support of libhat OS APIs on various platforms:

|                                | Windows | Linux | macOS |
|--------------------------------|:-------:|:-----:|:-----:|
| `hat::get_system`              |    ✅    |   ✅   |   ✅   |
| `hat::memory_protector`        |    ✅    |   ✅   |       |
| `hp::get_process_module`       |    ✅    |   ✅   |       |
| `hp::get_module`               |    ✅    |   ✅   |       |
| `hp::module_at`                |    ✅    |       |       |
| `hp::is_readable`              |    ✅    |   ✅   |       |
| `hp::is_writable`              |    ✅    |   ✅   |       |
| `hp::is_executable`            |    ✅    |   ✅   |       |
| `hp::module::get_module_data`  |    ✅    |       |       |
| `hp::module::get_section_data` |    ✅    |       |       |
| `hp::module::for_each_segment` |    ✅    |   ✅   |       |

## Quick start
### Defining patterns
libhat's signature syntax consists of space-delimited tokens and is backwards compatible with IDA syntax:

- 8 character sequences are interpreted as binary
- 2 character sequences are interpreted as hex
- 1 character must be a wildcard (`?`)

Any digit can be substituted for a wildcard, for example:
- `????1111` is a binary sequence, and matches any byte with all ones in the lower nibble
- `A?` is a hex sequence, and matches any byte of the form `1010????`
- Both `????????` and `??` are equivalent to `?`, and will match any byte

A complete pattern might look like `AB ? 12 ?3`. This matches any 4-byte
subrange `s` for which all the following conditions are met:
- `s[0] == 0xAB`
- `s[2] == 0x12`
- `s[3] & 0x0F == 0x03`

Due to how various scanning algorithms are implemented, there are some restrictions when defining a pattern:

1) A pattern must contain at least one fully masked byte (i.e. `AB` or `10011001`)
2) The first byte with a non-zero mask must have a full mask
   - `?1 02` is disallowed
   - `01 02` is allowed
   - `?? 01` is allowed

In code, there are a few ways to initialize a signature from its string representation:

```cpp
#include <libhat/scanner.hpp>

// Parse a pattern's string representation to an array of bytes at compile time
constexpr hat::fixed_signature pattern = hat::compile_signature<"48 8D 05 ? ? ? ? E8">();

// Parse using the UDLs at compile time
using namespace hat::literals;
constexpr hat::fixed_signature pattern = "48 8D 05 ? ? ? ? E8"_sig; // stack owned
constexpr hat::signature_view pattern = "48 8D 05 ? ? ? ? E8"_sigv; // static lifetime (requires C++23)

// Parse it at runtime
using parsed_t = hat::result<hat::signature, hat::signature_parse_error>;
parsed_t runtime_pattern = hat::parse_signature("48 8D 05 ? ? ? ? E8");
```

### Scanning patterns
```cpp
#include <libhat/scanner.hpp>

// Scan for this pattern using your CPU's vectorization features
auto begin = /* a contiguous iterator over std::byte */;
auto end = /* ... */;
hat::scan_result result = hat::find_pattern(begin, end, pattern);

// Scan a section in the process's base module
hat::scan_result result = hat::find_pattern(pattern, ".text");

// Or another module loaded into the process
std::optional<hat::process::module> ntdll = hat::process::get_module("ntdll.dll");
assert(ntdll.has_value());
hat::scan_result result = hat::find_pattern(pattern, ".text", *ntdll);

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

libhat has a few optimizations for searching for patterns in `x86_64` machine code:
```cpp
#include <libhat/scanner.hpp>

// If a byte pattern matches at the start of a function, the result will be aligned on 16-bytes.
// This can be indicated via the defaulted `alignment` parameter (all overloads have this parameter):
std::span<std::byte> range   = /* ... */;
hat::signature_view  pattern = /* ... */;
hat::scan_result     result  = hat::find_pattern(range, pattern, hat::scan_alignment::X16);

// Additionally, x86_64 contains a non-uniform distribution of byte pairs. By passing the `x86_64`
// scan hint, the search can be based on the least common byte pair that is found in the pattern.
hat::scan_result result  = hat::find_pattern(range, pattern, hat::scan_alignment::X1, hat::scan_hint::x86_64);
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
