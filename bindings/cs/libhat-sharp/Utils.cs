using Hat.Native;

namespace Hat;

public static class Utils
{
    public static void CheckStatus(Status status)
    {
        if (status == Status.Success) return;
        
        throw status switch
        {
            Status.EmptySig => new ArgumentException("Signature is empty."),
            Status.InvalidSig => new ArgumentException("Signature is invalid."),
            Status.NoBytesInSig => new ArgumentException("Signature contains no bytes, or only contains wildcards."),
            Status.UnknownError => new InvalidOperationException("Unknown error occurred."),
            _ => new ArgumentOutOfRangeException()
        };
    }
}