package me.zero.libhat.jna;

import com.sun.jna.*;
import com.sun.jna.ptr.PointerByReference;

/**
 * @author Brady
 */
public interface Libhat extends Library {

    Libhat INSTANCE = Native.load("libhat_c", Libhat.class);

    // ------------------------------------------------------------------------
    // C types
    // ------------------------------------------------------------------------

    class size_t extends IntegerType {
        private static final long serialVersionUID = 1L;

        public size_t() {
            this(0);
        }

        public size_t(long value) {
            super(Native.SIZE_T_SIZE, value, true);
        }
    }

    class uintptr_t extends IntegerType {
        private static final long serialVersionUID = 1L;

        public uintptr_t() {
            this(0);
        }

        public uintptr_t(long value) {
            super(Native.POINTER_SIZE, value, true);
        }
    }

    // ------------------------------------------------------------------------
    // Structures
    // ------------------------------------------------------------------------

    @Structure.FieldOrder({ "data", "size" })
    class Span extends Structure {
        public Pointer data;
        public size_t size;

        public Span() {}

        public Span(final Pointer p) {
            super(p);
            read();
        }

        public static class ByValue extends Span implements Structure.ByValue {}
    }

    // ------------------------------------------------------------------------
    // Callback types
    // ------------------------------------------------------------------------

    interface ForEachSectionCallback extends Callback {
        boolean invoke(String name, Span.ByValue data, int protection, Pointer userData);
    }

    interface ForEachSegmentCallback extends Callback {
        boolean invoke(Span.ByValue data, int protection, Pointer userData);
    }

    // ------------------------------------------------------------------------
    // API
    // ------------------------------------------------------------------------

    /*
     * libhat_status libhat_parse_signature(
     *     const char*              signatureStr,
     *     const libhat_signature** signatureOut
     * );
     */
    int libhat_parse_signature(String signatureStr, PointerByReference signatureOut);

    /*
     * libhat_status libhat_create_signature(
     *     const char*              bytes,
     *     const char*              mask,
     *     size_t                   size,
     *     const libhat_signature** signatureOut
     * );
     */
    int libhat_create_signature(byte[] bytes, byte[] mask, size_t size, PointerByReference signatureOut);

    /*
     * const void* libhat_find_pattern(
     *     const libhat_signature* signature,
     *     const void*             buffer,
     *     size_t                  size,
     *     libhat_alignment        align,
    *      libhat_hint             hints
     * );
     */
    Pointer libhat_find_pattern(Pointer signature, Pointer buffer, size_t size, int alignment, int hints);

    /*
     * const void* libhat_find_pattern_mod(
     *     const libhat_signature* signature,
     *     const libhat_module*    module,
     *     const char*             section,
     *     libhat_alignment        align,
     *     libhat_hint             hints
     * );
     */
    Pointer libhat_find_pattern_mod(Pointer signature, Pointer module, String section, int alignment, int hints);

    /*
     * uintptr_t libhat_module_address(const libhat_module* module);
     */
    uintptr_t libhat_module_address(Pointer module);

    /*
     * libhat_span libhat_module_get_data(const libhat_module* module);
     */
    Span libhat_module_get_data(Pointer module);

    /*
     * libhat_span libhat_module_get_executable_data(const libhat_module* module);
     */
    Span libhat_module_get_executable_data(Pointer module);

    /*
     * libhat_span libhat_module_get_section_data(const libhat_module* module, const char* name);
     */
    Span libhat_module_get_section_data(Pointer module, String name);

    /*
     * void libhat_module_for_each_section(
     *     const libhat_module*       module,
     *     libhat_for_each_section_cb callback,
     *     void*                      user_data
     * );
     */
    void libhat_module_for_each_section(Pointer module, ForEachSectionCallback callback, Pointer userData);

    /*
     * void libhat_module_for_each_segment(
     *     const libhat_module*       module,
     *     libhat_for_each_segment_cb callback,
     *     void*                      user_data
     * );
     */
    void libhat_module_for_each_segment(Pointer module, ForEachSegmentCallback callback, Pointer userData);

    /*
     * bool libhat_is_readable(const void* data, size_t size);
     */
    boolean libhat_is_readable(Pointer data, size_t size);

    /*
     * bool libhat_is_writable(const void* data, size_t size);
     */
    boolean libhat_is_writable(Pointer data, size_t size);

    /*
     * bool libhat_is_executable(const void* data, size_t size);
     */
    boolean libhat_is_executable(Pointer data, size_t size);

    /*
     * const libhat_module* libhat_get_process_module();
     */
    Pointer libhat_get_process_module();

    /*
     * const libhat_module* libhat_get_module(const char* name);
     */
    Pointer libhat_get_module(String name);

    /*
     * const libhat_module* libhat_module_at(const void* address);
     */
    Pointer libhat_module_at(Pointer address);

    /*
     * void libhat_free(const void* object);
     */
    void libhat_free(Pointer object);
}
