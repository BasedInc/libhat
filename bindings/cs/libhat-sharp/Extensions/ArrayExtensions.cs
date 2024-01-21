namespace Hat.Extensions;

public static class ArrayExtensions
{
	/// <summary>
	/// Creates a pattern from a nullable byte array, where null represents a wildcard.
	/// </summary>
	/// <returns>A pattern containing the bytes from this array.</returns>
	public static Pattern AsPattern(this IEnumerable<byte?> bytes)
	{
		return new Pattern(bytes);
	}
	
	/// <summary>
	/// Creates a pattern from a byte array.
	/// </summary>
	/// <returns>A pattern containing the bytes from this array.</returns>
	public static Pattern AsPattern(this IEnumerable<byte> bytes)
	{
		return new Pattern(bytes);
	}
}
