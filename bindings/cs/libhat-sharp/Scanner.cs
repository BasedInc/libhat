using Hat.Native;

namespace Hat;

public unsafe class Scanner
{
	private readonly string? _section;
	private readonly nint? _module;
	private readonly nint? _buffer;
	private readonly uint? _size;

	public Scanner(string section, nint module)
	{
		_section = section;
		_module = module;
	}
	
	public Scanner(nint buffer, uint size)
	{
		_buffer = buffer;
		_size = size;
	}
	
	public nint FindPattern(Pattern pattern, ScanAlignment alignment = ScanAlignment.X1)
	{
		if (_buffer is not null && _size is not null)
			return Functions.libhat_find_pattern(pattern.Signature, _buffer.Value, _size.Value, alignment);
		
		if (_section is not null && _module is not null)
			return Functions.libhat_find_pattern_mod(pattern.Signature, _module.Value, _section, alignment);
		
		throw new InvalidOperationException("Scanner is not initialized.");
	}
}