using System.Runtime.InteropServices;

namespace Hat.Native;

internal static unsafe partial class Functions
{
	private const string LIBRARY_NAME = "libhat_c";
	
	[LibraryImport(LIBRARY_NAME)]
	internal static partial Status libhat_parse_signature(
		[MarshalAs(UnmanagedType.LPStr)] string signatureStr, out Signature* signature);
	
	[LibraryImport(LIBRARY_NAME)]
	internal static partial Status libhat_create_signature(byte[] bytes, byte[] mask, uint size, out Signature* signature);
	
	[LibraryImport(LIBRARY_NAME)]
	internal static partial nint libhat_find_pattern(Signature* signature, nint buffer, uint size, ScanAlignment align);
	
	[LibraryImport(LIBRARY_NAME)]
	internal static partial nint libhat_find_pattern_mod(Signature* signature, nint module, 
		[MarshalAs(UnmanagedType.LPStr)] string section, ScanAlignment align);

	[LibraryImport(LIBRARY_NAME)]
	internal static partial void libhat_free(nint data);
}
