import ctypes
from typing import Callable

from ._ffi import _library, ForEachSectionCallback, ForEachSegmentCallback
from .enums import Protection
from .span import Span


class Module:
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

    def _check_handle(self) -> ctypes.c_void_p:
        if not self._handle:
            raise RuntimeError('Attempted operation on a Module that has already been freed')
        return self._handle

    def address(self) -> int:
        return _library.libhat_module_address(self._check_handle())

    def get_module_data(self) -> Span:
        return _library.libhat_module_get_data(self._check_handle())

    def get_executable_data(self) -> Span:
        return _library.libhat_module_get_executable_data(self._check_handle())

    def get_section_data(self, name: str) -> Span:
        return _library.libhat_module_get_section_data(self._check_handle(), name.encode('utf-8'))

    def for_each_section(self, callback: Callable[[str, Span, Protection], bool]):
        user_data = ctypes.cast(ctypes.pointer(ctypes.py_object(callback)), ctypes.c_void_p)
        _library.libhat_module_for_each_section(self._check_handle(), _section_trampoline, user_data)

    def for_each_segment(self, callback: Callable[[Span, Protection], bool]):
        user_data = ctypes.cast(ctypes.pointer(ctypes.py_object(callback)), ctypes.c_void_p)
        _library.libhat_module_for_each_segment(self._check_handle(), _segment_trampoline, user_data)


@ForEachSectionCallback
def _section_trampoline(name: bytes, data: Span, prot: int, user_data: int) -> bool:
    callback = ctypes.cast(user_data, ctypes.POINTER(ctypes.py_object)).contents.value
    return callback(name.decode('utf-8'), data, Protection(prot))


@ForEachSegmentCallback
def _segment_trampoline(data: Span, prot: int, user_data: int) -> bool:
    callback = ctypes.cast(user_data, ctypes.POINTER(ctypes.py_object)).contents.value
    return callback(data, Protection(prot))
