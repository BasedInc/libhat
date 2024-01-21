using Hat.Native;

namespace Hat;

public unsafe class Pattern : IDisposable
{
	internal readonly Signature* Signature;
	private bool _disposed;
	
	public Pattern(string signature)
	{
		var result = Functions.libhat_parse_signature(signature, out Signature);

		if (result == Status.Success) return;

		throw result switch
		{
			Status.EmptySig => new ArgumentException("Signature is empty.", nameof(signature)),
			Status.InvalidSig => new ArgumentException("Signature is invalid.", nameof(signature)),
			Status.NoBytesInSig => new ArgumentException("Signature contains no bytes, or only contains wildcards.",
				nameof(signature)),
			Status.UnknownError => new InvalidOperationException("Unknown error occurred."),
			_ => new ArgumentOutOfRangeException()
		};
	}

	public Pattern(IEnumerable<byte> signature)
	{
		var patternStr = signature.Select(b => b.ToString("X2")).Aggregate((a, b) => $"{a} {b}");
		
		var result = Functions.libhat_parse_signature(patternStr, out Signature);

		if (result == Status.Success) return;

		throw result switch
		{
			Status.EmptySig => new ArgumentException("Signature is empty.", nameof(signature)),
			Status.InvalidSig => new ArgumentException("Signature is invalid.", nameof(signature)),
			Status.NoBytesInSig => new ArgumentException("Signature contains no bytes, or only contains wildcards.",
				nameof(signature)),
			Status.UnknownError => new InvalidOperationException("Unknown error occurred."),
			_ => new ArgumentOutOfRangeException()
		};
	}

	public void Dispose()
	{
		if (_disposed)
			return;
		
		Functions.libhat_free((nint) Signature);
		_disposed = true;
		GC.SuppressFinalize(this);
	}
	
	~Pattern()
	{
		Dispose();
	}
}