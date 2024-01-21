using System.Runtime.InteropServices;

namespace Hat.Native;

[StructLayout(LayoutKind.Sequential)]
internal struct Signature
{
	public nint Data;
	public nint Length;
}
