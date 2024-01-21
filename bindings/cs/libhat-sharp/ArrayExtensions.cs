namespace Hat;

public static class ArrayExtensions
{
	public static Pattern AsPattern(this IEnumerable<byte?> bytes)
	{
		return new Pattern(bytes);
	}
	
	public static Pattern AsPattern(this IEnumerable<byte> bytes)
	{
		return new Pattern(bytes);
	}
}
