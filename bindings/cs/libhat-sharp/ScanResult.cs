using System.Numerics;

namespace Hat;

public unsafe class ScanResult
{
	/// <summary>
	/// The resulting address from the pattern scan.
	/// </summary>
	public nint Address { get; }
	
	internal ScanResult(nint address)
	{
		Address = address;
	}

	/// <summary>
	/// Reads a value of type <typeparamref name="T"/> located at a given offset.
	/// </summary>
	/// <param name="offset">The offset of the value. Defaults to 0.</param>
	public T Read<T>(int offset = 0) where T : unmanaged
	{
		return *(T*)(Address + offset);
	}

	/// <summary>
	/// Resolves a RIP relative address located at a given offset.
	/// </summary>
	/// <param name="offset">The offset of the relative address.</param>
	public nint Relative(int offset)
	{
		return Address + Read<int>(offset) + offset + sizeof(int);
	}
}