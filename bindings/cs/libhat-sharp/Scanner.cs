using System.Diagnostics;
using System.Runtime.CompilerServices;
using Hat.Native;

namespace Hat;

/// <summary>
/// Scans for a pattern in a process module or memory buffer.
/// </summary>
public unsafe class Scanner
{
	private readonly string? _section;
	private readonly nint? _module;
	private readonly nint? _buffer;
	private readonly uint? _size;

	/// <summary>
	/// Creates a scanner for a process module.
	/// </summary>
	/// <param name="module">The base address of the module.</param>
	/// <param name="section">The section to scan in. Defaults to .text.</param>
	public Scanner(nint module, string section = ".text")
	{
		_section = section;
		_module = module;
	}
	
	/// <summary>
	/// Creates a scanner for a memory buffer.
	/// </summary>
	/// <param name="buffer">The start address of the buffer.</param>
	/// <param name="size">The size of the buffer.</param>
	public Scanner(nint buffer, uint size)
	{
		_buffer = buffer;
		_size = size;
	}
	
	/// <summary>
	/// Creates a scanner for a <see cref="ProcessModule"/>.
	/// </summary>
	/// <param name="module">The module to scan in.</param>
	/// <param name="section">The section to scan in. Defaults to .text.</param>
	public Scanner(ProcessModule module, string section = ".text")
	{
		_section = section;
		_module = module.BaseAddress;
	}
	
	/// <summary>
	/// Creates a scanner for a <see cref="Span{T}"/> of bytes.
	/// </summary>
	/// <param name="buffer">The buffer to scan in.</param>
	public Scanner(Span<byte> buffer)
	{
		_buffer = (nint)Unsafe.AsPointer(ref buffer[0]);
		_size = (uint)buffer.Length;
	}
	
	/// <summary>
	/// Creates a scanner for a <see cref="Memory{T}"/> of bytes.
	/// </summary>
	/// <param name="buffer">The buffer to scan in.</param>
	public Scanner(Memory<byte> buffer) : this(buffer.Span) { }
	
	/// <summary>
	/// Scans for a pattern.
	/// </summary>
	/// <param name="pattern">The pattern to scan for.</param>
	/// <param name="alignment">The byte alignment of the result.</param>
	/// <returns>A <see cref="ScanResult"/> containing the result of the scan if found, otherwise null.</returns>
	public ScanResult? FindPattern(Pattern pattern, ScanAlignment alignment = ScanAlignment.X1)
	{
		if (_buffer is not null && _size is not null)
		{
			var result = Functions.libhat_find_pattern(pattern.Signature, _buffer.Value, _size.Value, alignment);
			return result == 0 ? null : new ScanResult(result);
		}

		if (_section is not null && _module is not null)
		{
			var result = Functions.libhat_find_pattern_mod(pattern.Signature, _module.Value, _section, alignment);
			return result == 0 ? null : new ScanResult(result);
		}
		
		throw new InvalidOperationException("Scanner is not initialized.");
	}
}
