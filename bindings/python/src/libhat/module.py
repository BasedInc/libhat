import ctypes
from collections.abc import Callable
from dataclasses import dataclass

from ._ffi import _library, c_uintptr, ForEachSectionCallback, ForEachSegmentCallback, Span
from .address import Address
from .enums import Protection
from .error import LibhatError


@dataclass(frozen=True)
class Section:
    name: str
    data: memoryview
    protection: Protection


@dataclass(frozen=True)
class Segment:
    data: memoryview
    protection: Protection


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

    def address(self) -> Address:
        result = c_uintptr()
        status = _library.libhat_module_address(self._check_handle(), ctypes.byref(result))
        if status != 0:
            raise LibhatError(status)
        return Address(result.value)

    def get_module_data(self) -> memoryview:
        result = Span()
        status = _library.libhat_module_get_data(self._check_handle(), ctypes.byref(result))
        if status != 0:
            raise LibhatError(status)
        return result.view()

    def get_executable_data(self) -> memoryview:
        result = Span()
        status = _library.libhat_module_get_executable_data(self._check_handle(), ctypes.byref(result))
        if status != 0:
            raise LibhatError(status)
        return result.view()

    def get_section_data(self, name: str) -> memoryview:
        result = Span()
        status = _library.libhat_module_get_section_data(self._check_handle(), name.encode('utf-8'),
                                                         ctypes.byref(result))
        if status != 0:
            raise LibhatError(status)
        return result.view()

    def for_each_section(self, callback: Callable[[Section], bool]):
        user_data = ctypes.cast(ctypes.pointer(ctypes.py_object(callback)), ctypes.c_void_p)
        status = _library.libhat_module_for_each_section(self._check_handle(), _section_trampoline, user_data)
        if status != 0:
            raise LibhatError(status)

    def for_each_segment(self, callback: Callable[[Segment], bool]):
        user_data = ctypes.cast(ctypes.pointer(ctypes.py_object(callback)), ctypes.c_void_p)
        status = _library.libhat_module_for_each_segment(self._check_handle(), _segment_trampoline, user_data)
        if status != 0:
            raise LibhatError(status)


@ForEachSectionCallback
def _section_trampoline(name: bytes, data: Span, prot: int, user_data: int) -> bool:
    callback = ctypes.cast(user_data, ctypes.POINTER(ctypes.py_object)).contents.value
    return callback(Section(name.decode('utf-8'), data.view(), Protection(prot)))


@ForEachSegmentCallback
def _segment_trampoline(data: Span, prot: int, user_data: int) -> bool:
    callback = ctypes.cast(user_data, ctypes.POINTER(ctypes.py_object)).contents.value
    return callback(Segment(data.view(), Protection(prot)))
