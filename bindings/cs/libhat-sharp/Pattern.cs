using Hat.Native;

namespace Hat;

/// <summary>
/// Represents a pattern, which can be used to scan for a sequence of bytes.
/// </summary>
public unsafe class Pattern : IDisposable
{
	internal Signature* Signature;
	
	/// <summary>
	/// Creates a pattern from its string representation, e.g. "48 8B 05 ? ? ? ? 48 8B 48 08".
	/// </summary>
	/// <param name="signature">
	/// The string representation of the pattern, where ? represents a wildcard.
	/// </param>
	/// <exception cref="ArgumentException">The signature is empty, invalid, or only contains wildcards.</exception>
	/// <exception cref="InvalidOperationException">An unknown error occurred.</exception>
	public Pattern(string signature)
	{
		Utils.CheckStatus(Functions.libhat_parse_signature(signature, out Signature));
	}
	
	/// <summary>
	/// Creates a pattern from a byte array.
	/// </summary>
	/// <param name="signature">An array of bytes.</param>
	/// <remarks>This function does not support wildcards. For a function that does, see <see cref="Pattern(IEnumerable{byte?})"/>.</remarks>
	/// <exception cref="ArgumentException">The signature is empty, invalid, or only contains wildcards.</exception>
	/// <exception cref="InvalidOperationException">An unknown error occurred.</exception>
	public Pattern(IEnumerable<byte> signature) : this(signature.Select(b => (byte?) b)) {}

	/// <summary>
	/// Creates a pattern from a nullable byte array, where null represents a wildcard.
	/// </summary>
	/// <param name="signature">An array of nullable bytes.</param>
	/// <exception cref="ArgumentException">The signature is empty, invalid, or only contains wildcards.</exception>
	/// <exception cref="InvalidOperationException">An unknown error occurred.</exception>
	public Pattern(IEnumerable<byte?> signature)
	{
		var arr = signature.ToArray();
		var bytes = arr.Select(b => b.GetValueOrDefault(0)).ToArray();
		var mask = arr.Select(b => (byte) (b.HasValue ? 0xFF : 0x00)).ToArray();
		Utils.CheckStatus(Functions.libhat_create_signature(bytes, mask, (uint) arr.Length, out Signature));
	}

	~Pattern()
	{
		Dispose();
	}

	/// <summary>
	/// Frees the memory allocated for the pattern.
	/// </summary>
	public void Dispose()
	{
		if (Signature == (Signature*)nint.Zero) return;
		
		Functions.libhat_free((nint) Signature);
		Signature = (Signature*)nint.Zero;
		GC.SuppressFinalize(this);
	}
}
