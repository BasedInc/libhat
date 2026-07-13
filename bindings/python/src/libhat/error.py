from ._ffi import _library

class LibhatError(Exception):
    def __init__(self, status: int):
        self.status = status
        super().__init__(_library.libhat_status_to_string(status).decode('utf-8'))
