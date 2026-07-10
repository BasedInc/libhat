package me.zero.libhat;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.nio.ByteBuffer;
import java.util.EnumSet;
import java.util.Objects;
import java.util.Optional;

/**
 * @author Brady
 */
public final class Section {

    @NotNull
    private final String name;

    @Nullable
    private final ByteBuffer data;

    @NotNull
    private final EnumSet<Protection> protection;

    Section(@NotNull final String name, @Nullable final ByteBuffer data, @NotNull final EnumSet<Protection> protection) {
        this.name = Objects.requireNonNull(name);
        this.data = data;
        this.protection = Objects.requireNonNull(protection);
    }

    public @NotNull String getName() {
        return this.name;
    }

    public @NotNull Optional<ByteBuffer> getData() {
        return Optional.ofNullable(this.data);
    }

    public @NotNull EnumSet<Protection> getProtection() {
        return this.protection;
    }
}
