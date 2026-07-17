# libhat

[![CMake](https://github.com/BasedInc/libhat/actions/workflows/cmake.yml/badge.svg)](https://github.com/BasedInc/libhat/actions)
[![LICENSE](https://img.shields.io/github/license/BasedInc/libhat?color=blue)](https://github.com/BasedInc/libhat/blob/master/LICENSE)
[![GitHub release](https://img.shields.io/github/v/release/BasedInc/libhat)](https://github.com/BasedInc/libhat/releases)
[![vcpkg port](https://img.shields.io/vcpkg/v/libhat)](https://vcpkg.link/ports/libhat)
[![xmake package](https://img.shields.io/badge/xmake-v0.10.0-orange?logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI2NCIgaGVpZ2h0PSI2NCIgeG1sbnM6dj0iaHR0cHM6Ly92ZWN0YS5pby9uYW5vIj48ZGVmcz48ZmlsdGVyIGlkPSJBIj48ZmVHYXVzc2lhbkJsdXIgaW49IlNvdXJjZUdyYXBoaWMiIHN0ZERldmlhdGlvbj0iLjUiLz48ZmVPZmZzZXQgZHg9Ii0wLjMiIGR5PSItMC4zIi8%2BPGZlTWVyZ2U%2BPGZlTWVyZ2VOb2RlLz48ZmVNZXJnZU5vZGUgaW49IlNvdXJjZUdyYXBoaWMiLz48L2ZlTWVyZ2U%2BPC9maWx0ZXI%2BPGNsaXBQYXRoIGlkPSJCIj48Y2lyY2xlIGN4PSIzMiIgY3k9IjMyIiByPSIyNyIvPjwvY2xpcFBhdGg%2BPHBhdGggaWQ9IkMiIGQ9Ik02NCA0djU4bC0yIDJIMWwtMS0xVjQwTDYwIDN6Ii8%2BPC9kZWZzPjxjaXJjbGUgIGN4PSIzMiIgY3k9IjMyIiByPSIzMCIgZmlsdGVyPSJ1cmwoI0EpIiBmaWxsPSJyZ2JhKDAsMCwwLDAuMikiLz48Y2lyY2xlICBjeD0iMzIiIGN5PSIzMiIgcj0iMzAiIGZpbGw9IiNlMGYyZjEiLz48ZyAgZmlsdGVyPSJ1cmwoI0EpIj48ZyAgY2xpcC1wYXRoPSJ1cmwoI0IpIj48ZyAgdHJhbnNmb3JtPSJ0cmFuc2xhdGUoMCA1KSI%2BPHVzZSAgaHJlZj0iI0MiIGZpbGw9IiNmZmYiLz48cGF0aCAgZD0iTTAgNjNsMSAxaDYxbDItMlY1MEw0IDEzbC0zLS41LTEgLjV6IiBmaWxsPSIjOGJjMzRhIi8%2BPHVzZSAgaHJlZj0iI0MiIGZpbGw9InJnYmEoMCwxNTAsMTM2LDAuNzYpIi8%2BPC9nPjwvZz48L2c%2BPC9zdmc%2B)](https://packages.xmake.io/packages/libhat)

A modern, high-performance C++20 library designed for game hacking in user space. Features an [incredibly](#Benchmarks)
fast SIMD-accelerated signature scanner, and a set of platform-agnostic APIs for interfacing with loaded modules,
modifying virtual memory protections, and more.

## Feature overview
- Vectorized scanning for byte patterns
  - SSE 4.1 and AVX2 on x86/x64
  - AVX-512 on x64
  - Neon on ARM/ARM64
- RAII memory protector
- Convenience wrappers over OS APIs
- Language bindings ([C](bindings/c), [Java](bindings/java), [Python](bindings/python), [C#](bindings/cs))
- Supports Windows, Linux, macOS, and Android
- General-purpose library features
  - `hat::cow`
  - `hat::cstring_view`
  - `hat::fixed_string`
  - `hat::string_literal`

## Versioning
This project adheres to [semantic versioning](https://semver.org/spec/v2.0.0.html). Any declaration that
is within a `detail` or `experimental` namespace is not considered part of the public API, and usage
may break at any time without the MAJOR version number being incremented.

## Integration

The best supported way to use libhat is via [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
or [CPM](https://github.com/cpm-cmake/CPM.cmake) in a CMake project:

```cmake
FetchContent_Declare(
    libhat
    GIT_REPOSITORY https://github.com/BasedInc/libhat.git
    GIT_TAG        v0.11.0
)
FetchContent_MakeAvailable(libhat)

target_link_libraries(my_target libhat::libhat)
```

```cmake
CPMAddPackage("gh:BasedInc/libhat#v0.11.0")

target_link_libraries(my_target libhat::libhat)
```

If you are using [xmake](https://xmake.io/), an [official package](https://github.com/xmake-io/xmake-repo/blob/dev/packages/l/libhat) is available:
```lua
add_requires("libhat v0.10.0")

target("my_target")
    add_packages("libhat")
```

If you are using [MSBuild](https://learn.microsoft.com/en-us/visualstudio/msbuild) or another build system, a
[vcpkg port](https://github.com/microsoft/vcpkg/tree/master/ports/libhat) is available:

```json
{
  "dependencies": [
    "libhat"
  ]
}
```

## Building

If you are exclusively building libhat from source, and do not need tests or examples, set the following options
when generating the buildsystem:

`-DLIBHAT_TESTING=OFF -DLIBHAT_EXAMPLES=OFF`

If you want to include the [C bindings](bindings/c) in your build, this must be done via the root CMake project. Specify
either `-DLIBHAT_STATIC_C_LIB=ON` or `-DLIBHAT_SHARED_C_LIB=ON` to build `libhat_c` as a static or shared library,
respectively. (Note that only one version of the library can be built per build configuration).

If you are using a multi-config generator, such as [Visual Studio](https://visualstudio.microsoft.com/):

```bash
cmake -B build
cmake --build build --config Release
cmake --install build --prefix install
```

If you are using a single-config generator, such as [Ninja](https://ninja-build.org/):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
cmake --install build --prefix install
```

Both options will output the headers, libraries, and binaries into a local `install` directory.

## Configuration

Currently, libhat only supports static linking (dynamic linking can be achieved using the [C bindings](bindings/c)). As
a result, features are configured through CMake options. By default, all features are enabled:

### `LIBHAT_FEATURE_SSE`

Enables support for the SSE vectorized find_pattern implementation, allowing CPUs that don't support newer instruction
sets (AVX families) to get higher throughput than the base standard library implementation. If compile-time support is
not present (`-march` or `/ARCH`), runtime support is validated through `cpuid`. If the target architecture is not `x86`
or `x86_64`, enabling this option has no effect.

CPU features: `sse4.1`

### `LIBHAT_FEATURE_AVX512`

Enables support for the AVX512 (BW+F) vectorized find_pattern implementation. If compile-time support is not present
(`-march` or `/ARCH`), runtime support is validated through `cpuid`. If the target architecture is not `x86_64`,
enabling this option has no effect.

CPU features: `avx512bw` `avx512f` `bmi`

### `LIBHAT_HINT_X86_64`

Enables support for `hat::scan_hint::x86_64`, which allows informed anchor selection when searching for patterns in
`x86_64` machine code, greatly reducing search time at the cost of a small data table (see [Benchmarks](#Benchmarks)).
This option is always supported regardless of the target architecture.

### `LIBHAT_HINT_AARCH64`

Enables support for `hat::scan_hint::aarch64`, which allows informed anchor selection when searching for patterns in
`aarch64` machine code, greatly reducing search time at the cost of a small data table. This option is always supported
regardless of the target architecture.

## Benchmarks
The table below compares the single threaded throughput in bytes/s (real time) between
libhat and [two other](test/benchmark/vendor) commonly used implementations for pattern
scanning. The input buffers were randomly generated using a fixed seed, and the pattern
scanned does not contain any match in the buffer. The benchmark was compiled on Windows
with `clang-cl` 22.1.1, using the MSVC 14.51.36231 toolchain and the default release mode
flags (`/GR /EHsc /MD /O2 /Ob2`). The benchmark was run on a system with an i7-14700K
(supporting [AVX2](src/arch/x86/AVX2.cpp)) and 64GB (4x16GB) DDR5 6000 MT/s (30-38-38-96).
The full source code is available [here](test/benchmark/Compare.cpp).
```
---------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations bytes_per_second
---------------------------------------------------------------------------------------------------
BM_Throughput_libhat/4MiB                    67015 ns        66389 ns        82139      58.2888Gi/s
BM_Throughput_libhat/16MiB                  311783 ns       310116 ns        18088      50.1151Gi/s
BM_Throughput_libhat/128MiB                5371248 ns      5355461 ns         1062      23.2721Gi/s
BM_Throughput_libhat/256MiB               10871393 ns     10787260 ns          520      22.9961Gi/s

BM_Throughput_std_search/4MiB              1372621 ns      1366807 ns         4104      2.84583Gi/s
BM_Throughput_std_search/16MiB             5412159 ns      5385316 ns         1030      2.88702Gi/s
BM_Throughput_std_search/128MiB           43333000 ns     43120155 ns          129      2.88464Gi/s
BM_Throughput_std_search/256MiB           86809503 ns     86181641 ns           64      2.87987Gi/s

BM_Throughput_std_find_std_equal/4MiB       189240 ns       189051 ns        29506      20.6418Gi/s
BM_Throughput_std_find_std_equal/16MiB      861429 ns       857686 ns         6613      18.1385Gi/s
BM_Throughput_std_find_std_equal/128MiB    9384011 ns      9385575 ns          591      13.3205Gi/s
BM_Throughput_std_find_std_equal/256MiB   18979502 ns     18939394 ns          297      13.1721Gi/s

BM_Throughput_UC1/4MiB                     1742814 ns      1732316 ns         3202      2.24135Gi/s
BM_Throughput_UC1/16MiB                    6983502 ns      6909938 ns          805      2.23742Gi/s
BM_Throughput_UC1/128MiB                  55866828 ns     55847772 ns          101      2.23746Gi/s
BM_Throughput_UC1/256MiB                 111606018 ns    111250000 ns           50      2.24002Gi/s

BM_Throughput_UC2/4MiB                     4080670 ns      4068654 ns         1371      980.231Mi/s
BM_Throughput_UC2/16MiB                   16422699 ns     16314338 ns          340      974.261Mi/s
BM_Throughput_UC2/128MiB                 132357452 ns    132440476 ns           42      967.078Mi/s
BM_Throughput_UC2/256MiB                 265336400 ns    264880952 ns           21      964.813Mi/s
```

Using the appropriate configuration, libhat is able to maintain its high throughput when searching 
machine code at a speed comparable to searching uniform buffers. The table below once again compares
the single threaded throughput in bytes/s (real time) against the same two alternative pattern scanners.
The buffer being scanned is `chrome.dll` from a Chromium
[snapshot](https://commondatastorage.googleapis.com/chromium-browser-snapshots/index.html?prefix=Win_x64/)
(~227MiB of code), and the pattern matches at the first instruction of `DllMain`.
The full source code is available [here](test/benchmark/Chromium.cpp).
```
---------------------------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations bytes_per_second
---------------------------------------------------------------------------------------------------
BM_find                                   36383054 ns     36254085 ns          153      6.09333Gi/s
BM_find_align                             11898416 ns     11876327 ns          471      18.6322Gi/s
BM_find_hint                               9687339 ns      9644397 ns          580      22.8849Gi/s
BM_find_align_hint                         9689014 ns      9636324 ns          574      22.8810Gi/s
BM_UC1                                   236431167 ns    236979167 ns           24      960.172Mi/s
BM_UC2                                   427872923 ns    427884615 ns           13      530.565Mi/s
```

## Platforms

Below is a summary of the current support for libhat's platform-dependent APIs:

### APIs

|                                   | Windows | Linux | macOS | Android |
|-----------------------------------|:-------:|:-----:|:-----:|:-------:|
| `hat::get_system`                 |    ✅    |   ✅   |   ✅   |    ✅    |
| `hat::memory_protector`           |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::get_process_module`          |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::get_module`                  |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module_at`                   |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::is_readable`                 |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::is_writable`                 |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::is_executable`               |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::get_symbol`          |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::get_module_data`     |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::get_executable_data` |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::get_section_data`    |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::for_each_section`    |    ✅    |   ✅   |   ✅   |    ✅    |
| `hp::module::for_each_segment`    |    ✅    |   ✅   |   ✅   |    ✅    |

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

As a scanning optimization, all patterns are required to have at least one fully masked byte. Attempting to find a
pattern that does not meet this requirement will result in undefined behavior. Additionally, it is recommended
(but not required) that patterns contain at least 2 consecutive fully masked bytes, as this will greatly speed
up the vectorized scanning algorithms.
- `?1 02` is allowed
- `?? 02` is allowed
- `01 02` is allowed (*and recommended*)

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

libhat has a few optimizations for searching for patterns in `x86_64` and `AArch64` machine code:
```cpp
#include <libhat/scanner.hpp>

// Compilers will often align the start address of a function on 16-bytes. Scanning for patterns that
// match the start of a function can take advantage of this by specifying the defaulted `alignment`
// parameter (all overloads have this parameter):
std::span<std::byte> range   = /* ... */;
hat::signature_view  pattern = /* ... */;
hat::scan_result     result  = hat::find_pattern(range, pattern, hat::scan_alignment::X16);

// Or, if the architecture has byte-aligned instructions (such as ARM and AArch64):
hat::scan_result result = hat::find_pattern(range, pattern, hat::scan_alignment::X4);

// Additionally, machine code contains a non-uniform distribution of bytes. By passing the respective
// scan hint (either `x86_64` or `aarch64`), the search anchor can be tuned to the least frequent
// bytes that are present in the pattern.
hat::scan_result result = hat::find_pattern(range, pattern, hat::scan_alignment::X1, hat::scan_hint::x86_64);
```

### Accessing members
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
