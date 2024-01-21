package me.zero.libhat;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import me.zero.libhat.jna.Libhat;

import java.nio.ByteBuffer;
import java.util.OptionalInt;

/**
 * @author Brady
 */
public final class Hat {

    private Hat() {}

    public enum Status {
        SUCCESS,
        ERR_UNKNOWN,
        SIG_INVALID,
        SIG_EMPTY,
        SIG_NO_BYTE
    }

    public static Signature parseSignature(final String signature) {
        final Pointer[] handle = new Pointer[1];
        final int status = Libhat.INSTANCE.libhat_parse_signature(signature, handle);
        if (status != 0) {
            throw new RuntimeException("libhat internal error " + Status.values()[status]);
        }
        return new Signature(handle[0]);
    }

    public static Signature createSignature(byte[] bytes, byte[] mask) {
        if (bytes.length != mask.length) {
            throw new IllegalArgumentException("Mismatch between bytes.length and mask.length");
        }
        final Pointer[] handle = new Pointer[1];
        final int status = Libhat.INSTANCE.libhat_create_signature(bytes, mask, bytes.length, handle);
        if (status != 0) {
            throw new RuntimeException("libhat internal error " + Status.values()[status]);
        }
        return new Signature(handle[0]);
    }

    public static OptionalInt findPattern(final Signature signature, final ByteBuffer buffer) {
        return findPattern(signature, buffer, ScanAlignment.X1);
    }

    public static OptionalInt findPattern(final Signature signature, final ByteBuffer buffer, final ScanAlignment alignment) {
        if (!buffer.isDirect()) {
            throw new IllegalArgumentException("Provided buffer must be direct");
        }

        final long start = Pointer.nativeValue(Native.getDirectBufferPointer(buffer)) + buffer.position();
        final int count = buffer.remaining();

        final Pointer result = Libhat.INSTANCE.libhat_find_pattern(
            signature.handle,
            new Pointer(start),
            count,
            alignment.ordinal()
        );

        if (result == Pointer.NULL) {
            return OptionalInt.empty();
        }
        return OptionalInt.of((int) (Pointer.nativeValue(result) - start) + buffer.position());
    }
}
