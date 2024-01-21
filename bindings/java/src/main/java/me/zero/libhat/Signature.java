package me.zero.libhat;

import com.sun.jna.Pointer;
import me.zero.libhat.jna.Libhat;

/**
 * @author Brady
 */
public final class Signature implements AutoCloseable {

    Pointer handle;

    Signature(Pointer handle) {
        this.handle = handle;
    }

    @Override
    public void close() {
        if (this.handle != Pointer.NULL) {
            Libhat.INSTANCE.libhat_free(this.handle);
            this.handle = Pointer.NULL;
        }
    }
}
