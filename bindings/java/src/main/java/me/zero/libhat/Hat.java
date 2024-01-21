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

    /**
     * Creates a new {@link Signature} from the specified string representation of a byte pattern. For example:
     * <ul>
     *     <li><code>48 8B 8D ? ? ? ? 48</code></li>
     *     <li><code>E8 ? ? ? ? 88 86</code></li>
     * </ul>
     * The returned {@link Signature} is backed by a native heap allocation, and {@link Signature#close()} must be
     * called when the object is done being used, either explicitly or through a try-with-resources block.
     *
     * @param signature The string representation of a byte pattern
     * @return The created signature
     * @throws RuntimeException if an internal error occurred, indicated by {@code status != Status.SUCCESS}
     */
    public static Signature parseSignature(final String signature) {
        final Pointer[] handle = new Pointer[1];
        final int status = Libhat.INSTANCE.libhat_parse_signature(signature, handle);
        if (status != 0) {
            throw new RuntimeException("libhat internal error " + Status.values()[status]);
        }
        return new Signature(handle[0]);
    }

    /**
     * Creates a new {@link Signature} that describes a pattern matching the specified {@code bytes} where the
     * corresponding {@code mask} value is non-zero. In other words, a mask value of {@code 0} indicates a wildcard.
     * The returned {@link Signature} is backed by a native heap allocation, and {@link Signature#close()} must be
     * called when the object is done being used, either explicitly or through a try-with-resources block.
     *
     * @param bytes The bytes to match
     * @param mask  The mask applied to bytes
     * @return The created signature
     * @throws IllegalArgumentException if {@code bytes.length != mask.length}
     * @throws RuntimeException if an internal error occurred, indicated by {@code status != Status.SUCCESS}
     */
    public static Signature createSignature(final byte[] bytes, final byte[] mask) {
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

    /**
     * Finds the byte pattern described by the given {@link Signature} in the specified {@code buffer}, searching the
     * range including {@link ByteBuffer#position()} and up to but excluding {@link ByteBuffer#limit()}. If a match is
     * found, an {@link OptionalInt} containing the absolute position into {@code buffer} is returned. The underlying
     * memory address of the returned result will not be aligned on any particular boundary. The specified
     * {@link ByteBuffer} must be a direct buffer.
     *
     * @param signature The pattern to match
     * @param buffer    The buffer to search
     * @return The absolute position into {@code buffer} where a match was found,
     *         or {@link OptionalInt#empty()} if there was no match
     * @throws IllegalArgumentException if the buffer is not direct or the signature has already been closed
     */
    public static OptionalInt findPattern(final Signature signature, final ByteBuffer buffer) {
        return findPattern(signature, buffer, ScanAlignment.X1);
    }

    /**
     * Finds the byte pattern described by the given {@link Signature} in the specified {@code buffer}, searching the
     * range including {@link ByteBuffer#position()} and up to but excluding {@link ByteBuffer#limit()}. If a match is
     * found, an {@link OptionalInt} containing the absolute position into {@code buffer} is returned. The underlying
     * memory address of the returned result will be byte aligned per the specified {@link ScanAlignment}. The specified
     * {@link ByteBuffer} must be a direct buffer.
     *
     * @param signature The pattern to match
     * @param buffer    The buffer to search
     * @param alignment The result address alignment
     * @return The absolute position into {@code buffer} where a match was found,
     *         or {@link OptionalInt#empty()} if there was no match
     * @throws IllegalArgumentException if the buffer is not direct or the signature has already been closed
     */
    public static OptionalInt findPattern(final Signature signature, final ByteBuffer buffer, final ScanAlignment alignment) {
        if (!buffer.isDirect()) {
            throw new IllegalArgumentException("Provided buffer must be direct");
        }
        if (signature.handle == Pointer.NULL) {
            throw new IllegalArgumentException("signature was nullptr");
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

    /**
     * Wrapper around {@link #parseSignature(String)} and {@link #findPattern(Signature, ByteBuffer)}
     *
     * @param signature A byte pattern string
     * @param buffer The buffer to search
     * @return The search result
     */
    public static OptionalInt findPattern(final String signature, final ByteBuffer buffer) {
        try (final Signature sig = parseSignature(signature)) {
            return findPattern(sig, buffer);
        }
    }

    /**
     * Wrapper around {@link #parseSignature(String)} and {@link #findPattern(Signature, ByteBuffer, ScanAlignment)}
     *
     * @param signature A byte pattern string
     * @param buffer The buffer to search
     * @param alignment The memory address alignment of the result
     * @return The search result
     */
    public static OptionalInt findPattern(final String signature, final ByteBuffer buffer, final ScanAlignment alignment) {
        try (final Signature sig = parseSignature(signature)) {
            return findPattern(sig, buffer, alignment);
        }
    }
}
