package me.zero.libhat;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import me.zero.libhat.jna.Libhat;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.nio.ByteBuffer;
import java.util.Objects;
import java.util.Optional;
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
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static @NotNull Signature parseSignature(@NotNull final String signature) {
        Objects.requireNonNull(signature);
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
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static @NotNull Signature createSignature(final byte @NotNull[] bytes, final byte @NotNull[] mask) {
        Objects.requireNonNull(bytes);
        Objects.requireNonNull(mask);
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
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static OptionalInt findPattern(@NotNull final Signature signature, @NotNull final ByteBuffer buffer) {
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
     * @throws IllegalArgumentException if the buffer is not direct
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static OptionalInt findPattern(@NotNull final Signature signature, @NotNull final ByteBuffer buffer,
                                          @NotNull final ScanAlignment alignment) {
        Objects.requireNonNull(signature);
        Objects.requireNonNull(buffer);
        Objects.requireNonNull(alignment);

        if (!buffer.isDirect()) {
            throw new IllegalArgumentException("Provided buffer must be direct");
        }

        final long start = Pointer.nativeValue(Native.getDirectBufferPointer(buffer)) + buffer.position();
        final int count = buffer.remaining();

        final Pointer result = Libhat.INSTANCE.libhat_find_pattern(
            Objects.requireNonNull(signature.handle),
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
     * Finds the byte pattern described by the given {@link Signature} in the specified {@code section} of the
     * specified {@code module}. If a match is found, an {@link Optional} containing a Pointer to the match is returned.
     * The underlying memory address of the returned result will not be aligned on any particular boundary.
     *
     * @param signature The pattern to match
     * @param module    The target module
     * @param section   The section to search in the module
     * @return A pointer to the memory where a match was identified, or {@link Optional#empty()} if none was found.
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static Optional<Pointer> findPattern(@NotNull final Signature signature, @NotNull final ProcessModule module,
                                                @NotNull final String section) {
        return findPattern(signature, module, section, ScanAlignment.X1);
    }

    /**
     * Finds the byte pattern described by the given {@link Signature} in the specified {@code section} of the
     * specified {@code module}. If a match is found, an {@link Optional} containing a Pointer to the match is returned.
     * The underlying memory address of the returned result will be byte aligned per the specified {@link ScanAlignment}.
     *
     * @param signature The pattern to match
     * @param module    The target module
     * @param section   The section to search in the module
     * @param alignment The result address alignment
     * @return A pointer to the memory where a match was identified, or {@link Optional#empty()} if none was found.
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static Optional<Pointer> findPattern(@NotNull final Signature signature, @NotNull final ProcessModule module,
                                                @NotNull final String section, @NotNull final ScanAlignment alignment) {
        Objects.requireNonNull(signature);
        Objects.requireNonNull(module);
        Objects.requireNonNull(section);
        Objects.requireNonNull(alignment);

        final Pointer result = Libhat.INSTANCE.libhat_find_pattern_mod(
            Objects.requireNonNull(signature.handle),
            module.handle,
            section,
            alignment.ordinal()
        );

        return Optional.ofNullable(result);
    }

    /**
     * Wrapper around {@link #parseSignature(String)} and {@link #findPattern(Signature, ByteBuffer)}
     *
     * @param signature A byte pattern string
     * @param buffer The buffer to search
     * @return The search result
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static OptionalInt findPattern(@NotNull final String signature, @NotNull final ByteBuffer buffer) {
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
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static OptionalInt findPattern(@NotNull final String signature, @NotNull final ByteBuffer buffer,
                                          @NotNull final ScanAlignment alignment) {
        try (final Signature sig = parseSignature(signature)) {
            return findPattern(sig, buffer, alignment);
        }
    }

    /**
     * Returns the module for the executable used to create this process
     *
     * @return The module
     * @throws IllegalStateException If the process module could not be retrieved
     * @throws NullPointerException if any arguments are {@code null}
     */
    public static @NotNull ProcessModule getProcessModule() {
        return getModule(null).orElseThrow(() -> new IllegalStateException("Process module was nullptr"));
    }

    /**
     * Returns an {@link Optional} containing a handle to the module with the specified name, if such a module exists,
     * otherwise the returned optional is empty. If the provided name is {@code null}, the module for the executable
     * used to create the current process is returned.
     *
     * @param module The module name, may be {@code null}
     * @return The module
     */
    public static Optional<ProcessModule> getModule(@Nullable final String module) {
        final Pointer addr = Libhat.INSTANCE.libhat_get_module(module);
        if (addr == Pointer.NULL) {
            return Optional.empty();
        }
        return Optional.of(new ProcessModule(addr));
    }
}
