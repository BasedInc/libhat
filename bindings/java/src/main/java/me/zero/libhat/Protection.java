package me.zero.libhat;

import org.jetbrains.annotations.NotNull;

import java.util.EnumSet;

/**
 * @author Brady
 */
public enum Protection {
    READ,
    WRITE,
    EXECUTE;

    private final int bit;

    Protection() {
        this.bit = (1 << this.ordinal());
    }

    public static @NotNull EnumSet<Protection> fromFlags(final int flags) {
        final EnumSet<Protection> set = EnumSet.noneOf(Protection.class);
        for (final Protection flag : Protection.values()) {
            if ((flags & flag.bit) != 0) {
                set.add(flag);
            }
        }
        return set;
    }
}
