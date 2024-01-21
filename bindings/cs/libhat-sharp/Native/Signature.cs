using System.Runtime.InteropServices;

namespace Hat.Native;

[StructLayout(LayoutKind.Sequential)]
public struct Signature
{
	public nint Data;
	public uint Length;
}