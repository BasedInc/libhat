package me.zero.libhat;

import org.jetbrains.annotations.NotNull;

/**
 * @author Brady
 */
public enum ScanHint {
    /**
     * The data being scanned is x86_64 machine code
     */
    X86_64,

    /**
     *  Only utilize byte pair based scanning if the signature starts with a byte pair
     */
    PAIR0,

    /**
     * The data being scanned is AArch64 machine code
     */
    AARCH64;

    public static int mask(@NotNull ScanHint... hints) {
        int mask = 0;
        for (final ScanHint hint : hints) {
            mask |= (1 << hint.ordinal());
        }
        return mask;
    }
}
