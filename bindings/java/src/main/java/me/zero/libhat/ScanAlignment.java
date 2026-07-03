package me.zero.libhat;

/**
 * @author Brady
 */
public enum ScanAlignment {
    /**
     * No byte alignment
     */
    X1(1),

    /**
     * 4 byte alignment
     */
    X4(4),

    /**
     * 16 byte alignment
     */
    X16(16);

    private final int alignment;

    ScanAlignment(final int alignment) {
        this.alignment = alignment;
    }

    /**
     * @return The scan result alignment requirement, in bytes
     */
    public final int alignment() {
        return this.alignment;
    }
}
