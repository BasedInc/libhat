package me.zero.libhat;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.PointerByReference;
import me.zero.libhat.jna.Libhat;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.nio.ByteBuffer;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Predicate;

/**
 * @author Brady
 */
public final class ProcessModule implements AutoCloseable {

    @Nullable
    Pointer handle;

    ProcessModule(@NotNull final Pointer handle) {
        this.handle = Objects.requireNonNull(handle);
    }

    @Override
    public void close() {
        if (this.handle != Pointer.NULL) {
            Libhat.INSTANCE.libhat_free(this.handle);
            this.handle = Pointer.NULL;
        }
    }

    /**
     * @return The base address of this module, as a {@code long}.
     * @throws LibhatException       if an internal error occurred
     */
    public long getBaseAddress() {
        PointerByReference out = new PointerByReference();
        final int status = Libhat.INSTANCE.libhat_module_address(this.checkHandle(), out);
        if (status != 0) {
            throw new LibhatException(status);
        }
        return Pointer.nativeValue(out.getValue());
    }

    /**
     * Returns the complete memory region for this module. This may include portions which are uncommitted.
     * To verify whether the region is safe to read, use {@link Hat#isReadable(ByteBuffer)}.
     *
     * @return The module data
     * @throws IllegalStateException if the module data was empty
     * @throws LibhatException       if an internal error occurred
     */
    public @NotNull ByteBuffer getModuleData() {
        final Libhat.Span.ByReference out = new Libhat.Span.ByReference();
        final int status = Libhat.INSTANCE.libhat_module_get_data(this.checkHandle(), out);
        if (status != 0) {
            throw new LibhatException(status);
        }

        final ByteBuffer data = out.toBuffer();
        if (data == null) {
            throw new IllegalStateException("Module data was unexpectedly empty");
        }
        return data;
    }

    /**
     * Returns the executable memory region containing machine code for the module. The standard section which
     * contains executable code for the current platform will be returned first. If it cannot be identified
     * by name, the first executable region defined by the module will be returned instead.
     *
     * @return The module's executable data, or {@link Optional#empty()} if it cannot be found.
     * @throws LibhatException if an internal error occurred
     */
    public @NotNull Optional<ByteBuffer> getExecutableData() {
        final Libhat.Span.ByReference out = new Libhat.Span.ByReference();
        final int status = Libhat.INSTANCE.libhat_module_get_executable_data(this.checkHandle(), out);
        if (status != 0) {
            throw new LibhatException(status);
        }

        return Optional.ofNullable(out.toBuffer());
    }

    /**
     * Returns the memory region for a named section. On Linux-based platforms, section names are lazily loaded
     * from the file that initialized the module, and internally cached for subsequent calls. On systems using the
     * Mach-O format, "SEGNAME,SECNAME" is supported for disambiguation. i.e. "__TEXT,__const" vs "__DATA,__const"
     *
     * @param name The section name
     * @return The data for the section, or {@link Optional#empty()} if it cannot be found.
     * @throws NullPointerException if any arguments are {@code null}
     * @throws LibhatException      if an internal error occurred
     */
    public @NotNull Optional<ByteBuffer> getSectionData(@NotNull final String name) {
        Objects.requireNonNull(name);

        final Libhat.Span.ByReference out = new Libhat.Span.ByReference();
        final int status = Libhat.INSTANCE.libhat_module_get_section_data(this.checkHandle(), name, out);
        if (status != 0) {
            throw new LibhatException(status);
        }

        return Optional.ofNullable(out.toBuffer());
    }

    /**
     * Invokes the callback for each named linker section defined by this module as long as it returns true. The
     * returned byte range is not guaranteed to have page aligned begin and end addresses. The returned protections
     * are yielded from the section headers, and may not reflect the current virtual protections for the relevant
     * memory pages. On Linux-based platforms, section names are lazily loaded from the file that initialized the
     * module, and internally cached for subsequent calls.
     *
     * @param callback The callback to accept each section
     * @throws NullPointerException if any arguments are {@code null}
     * @throws LibhatException      if an internal error occurred
     */
    public void forEachSection(@NotNull final Predicate<@NotNull Section> callback) {
        Objects.requireNonNull(callback);
        final int status = Libhat.INSTANCE.libhat_module_for_each_section(this.checkHandle(), (name, data, prot, ud)
            -> callback.test(new Section(name, data.toBuffer(), Protection.fromFlags(prot))), null);
        if (status != 0) {
            throw new LibhatException(status);
        }
    }

    /**
     * Invokes the callback for each memory segment defined by this module as long as it returns true. Depending on
     * the platform, a segment may be represented by multiple linker sections. The returned byte range is not
     * guaranteed to have page aligned begin and end addresses. The returned protections are yielded from the
     * segment headers, and may not reflect the current virtual protections for the relevant memory pages.
     *
     * @param callback The callback to accept each segment
     * @throws NullPointerException if any arguments are {@code null}
     * @throws LibhatException      if an internal error occurred
     */
    public void forEachSegment(@NotNull final Predicate<@NotNull Segment> callback) {
        Objects.requireNonNull(callback);
        final int status = Libhat.INSTANCE.libhat_module_for_each_segment(this.checkHandle(), (data, prot, ud)
            -> callback.test(new Segment(data.toBuffer(), Protection.fromFlags(prot))), null);
        if (status != 0) {
            throw new LibhatException(status);
        }
    }

    @NotNull
    private Pointer checkHandle() {
        return Objects.requireNonNull(this.handle,
            "Attempted operation on a ProcessModule that has already been freed");
    }
}
