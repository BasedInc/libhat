import ctypes

from ._ffi import _library


class Signature:
    def __init__(self, handle: ctypes.c_void_p):
        if not handle:
            raise ValueError('handle must not be nullptr')
        self._handle = handle

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()

    def close(self):
        if self._handle:
            _library.libhat_free(self._handle)
            self._handle = None

    @property
    def handle(self):
        return self._handle
