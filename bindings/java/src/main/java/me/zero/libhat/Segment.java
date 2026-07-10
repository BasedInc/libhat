package me.zero.libhat;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.nio.ByteBuffer;
import java.util.EnumSet;
import java.util.Optional;

/**
 * @author Brady
 */
public final class Segment {

    @Nullable
    private final ByteBuffer data;

    @NotNull
    private final EnumSet<Protection> protection;

    public Segment(@Nullable final ByteBuffer data, @NotNull final EnumSet<Protection> protection) {
        this.data = data;
        this.protection = protection;
    }

    public @NotNull Optional<ByteBuffer> getData() {
        return Optional.ofNullable(data);
    }

    public @NotNull EnumSet<Protection> getProtection() {
        return protection;
    }
}
