using Hat.Native;

namespace Hat;

public unsafe class Pattern : IDisposable
{
	internal Signature* Signature;
	
	public Pattern(string signature)
	{
		Utils.CheckStatus(Functions.libhat_parse_signature(signature, out Signature));
	}
	
	public Pattern(IEnumerable<byte> signature) : this(signature.Select(b => (byte?) b)) {}

	public Pattern(IEnumerable<byte?> signature)
	{
		byte?[] arr = signature.ToArray();
		byte[] bytes = arr.Select(b => b.GetValueOrDefault(0)).ToArray();
		byte[] mask = arr.Select(b => (byte) (b.HasValue ? 0xFF : 0x00)).ToArray();
		Utils.CheckStatus(Functions.libhat_create_signature(bytes, mask, (uint) arr.Length, out Signature));
	}

	~Pattern()
	{
		Dispose();
	}

	public void Dispose()
	{
		if (Signature == (Signature*)nint.Zero) return;
		
		Functions.libhat_free((nint) Signature);
		Signature = (Signature*)nint.Zero;
		GC.SuppressFinalize(this);
	}
}
