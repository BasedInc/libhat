package me.zero.libhat;

import com.sun.jna.Pointer;
import me.zero.libhat.jna.Libhat;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

import java.util.Objects;

/**
 * @author Brady
 */
public final class Signature implements AutoCloseable {

    @Nullable
    Pointer handle;

    Signature(@NotNull final Pointer handle) {
        this.handle = Objects.requireNonNull(handle);
    }

    @Override
    public void close() {
        if (this.handle != Pointer.NULL) {
            Libhat.INSTANCE.libhat_free(this.handle);
            this.handle = Pointer.NULL;
        }
    }
}
