namespace Hat;

/// <summary>
/// Represents the result of a pattern scan.
/// </summary>
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
	/// <param name="remaining">The amount of bytes remaining after the relative address.</param>
	/// <remarks>
	/// In some cases, the instruction pointed at by the result does not end with the relative address.
	/// To prevent incorrect results caused by the remaining bytes, the <paramref name="remaining"/> parameter
	/// should be used to specify the amount of bytes remaining in the instruction.
	/// </remarks>
	public nint Relative(int offset, int remaining = 0)
	{
		return Address + Read<int>(offset) + offset + sizeof(int) + remaining;
	}
}