package me.zero.libhat.jna;

import com.sun.jna.*;

/**
 * @author Brady
 */
public interface Libhat extends Library {

    Libhat INSTANCE = Native.load("libhat_c", Libhat.class);

    /*
     * libhat_status_t libhat_parse_signature(
     *     const char*   signatureStr,
     *     signature_t** signatureOut
     * );
     */
    int libhat_parse_signature(String signatureStr, Pointer[] signatureOut);

    /*
     * libhat_status_t libhat_create_signature(
     *     const char*   bytes,
     *     const char*   mask,
     *     size_t        size,
     *     signature_t** signatureOut
     * );
     */
    int libhat_create_signature(byte[] bytes, byte[] mask, int size, Pointer[] signatureOut);

    /*
     * const void* libhat_find_pattern(
     *     const signature_t*  signature,
     *     const void*         buffer,
     *     size_t              size,
     *     scan_alignment      align
     * );
     */
    Pointer libhat_find_pattern(Pointer signature, Pointer buffer, long size, int align);

    /*
     * const void* libhat_find_pattern_mod(
     *     const signature_t*  signature,
     *     const void*         module,
     *     const char*         section,
     *     scan_alignment      align
     * );
     */
    Pointer libhat_find_pattern_mod(Pointer signature, Pointer module, String section, int align);

    /*
     * void libhat_free(void* mem);
     */
    void libhat_free(Pointer mem);
}
