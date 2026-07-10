package me.zero.libhat;

import me.zero.libhat.jna.Libhat;

/**
 * @author Brady
 */
public final class LibhatException extends RuntimeException {

    private final int status;

    LibhatException(final int status) {
        super(Libhat.INSTANCE.libhat_status_to_string(status));
        this.status = status;
    }

    /**
     * @return The actual status code representing the error that occurred
     */
    public int status() {
        return this.status;
    }
}
