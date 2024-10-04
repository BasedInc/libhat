# libhat-sharp
C# bindings for libhat, a high-performance game hacking library.

## Notes
- Currently only includes the pattern scanning functionality of libhat (libhat_c only implements pattern scanning)
- Supports Windows only, x86 and x64
- Linux support will be added in the future

## Installation
The library is available on NuGet: https://www.nuget.org/packages/libhat-sharp/

You can install it using the following command:
`dotnet add package libhat-sharp`

## Usage
```csharp
using Hat;

// Parse a pattern's string representation into a byte array at runtime
Pattern pattern = new Pattern("48 8D 05 ? ? ? ? E8");

// Create a scanner object for a section in a specific module
Process process = /* ... */;
Scanner scanner = new Scanner(process.MainModule, ".text");

// ...or for a specific memory region
nint start = /* the start of the memory region you want to scan */;
uint size = /* ... */;
Scanner scanner = new Scanner(start, size);

// A span can also be used
Span<byte> buffer = /* ... */;
Scanner scanner = new Scanner(buffer);

// Scan for this pattern using your CPU's vectorization features
nint result = scanner.FindPattern(pattern);