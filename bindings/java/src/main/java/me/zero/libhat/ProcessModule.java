package me.zero.libhat;

import com.sun.jna.Pointer;
import org.jetbrains.annotations.NotNull;

/**
 * @author Brady
 */
public final class ProcessModule {

    @NotNull
    Pointer handle;

    ProcessModule(@NotNull final Pointer handle) {
        this.handle = handle;
    }
}
