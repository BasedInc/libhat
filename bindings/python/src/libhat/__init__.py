import ctypes
from contextlib import nullcontext

from ._ffi import _library, Span
from .address import Address
from .enums import Protection, ScanAlignment, ScanHint
from .error import LibhatError
from .module import Module, Section, Segment
from .signature import Signature

__all__ = [
    # functions
    'get_version',
    'get_version_num',
    'parse_signature',
    'create_signature',
    'find_pattern',
    'find_pattern_mod',
    'is_readable',
    'is_writable',
    'is_executable',
    'get_process_module',
    'get_module',
    'module_at',

    # types
    'Signature',
    'Module',
    'Section',
    'Segment',
    'Span',
    'Protection',
    'ScanAlignment',
    'ScanHint',
    'Address',
    'LibhatError',
]


def get_version() -> str:
    return _library.libhat_get_version().decode('utf-8')


def get_version_num() -> int:
    return _library.libhat_get_version_num()


def parse_signature(signature_str: str) -> Signature:
    handle = ctypes.c_void_p()
    status = _library.libhat_parse_signature(signature_str.encode('utf-8'), ctypes.byref(handle))
    if status != 0:
        raise LibhatError(status)
    return Signature(handle)


def create_signature(sig_bytes: bytes, sig_mask: bytes) -> Signature:
    if len(sig_bytes) != len(sig_mask):
        raise ValueError('Mismatch between len(sig_bytes) and len(sig_mask)')
    handle = ctypes.c_void_p()
    status = _library.libhat_create_signature(sig_bytes, sig_mask, len(sig_mask), ctypes.byref(handle))
    if status != 0:
        raise LibhatError(status)
    return Signature(handle)


def find_pattern(sig: str | Signature, buf: bytes | bytearray | Span, align: ScanAlignment = ScanAlignment.X1,
                 hints: ScanHint = ScanHint(0)) -> int | None:
    if isinstance(buf, bytes):
        data, size = ctypes.cast(buf, ctypes.c_void_p), len(buf)
    elif isinstance(buf, bytearray):
        data = ctypes.cast((ctypes.c_ubyte * len(buf)).from_buffer(buf), ctypes.c_void_p)
        size = len(buf)
    elif isinstance(buf, Span):
        data, size = ctypes.c_void_p(buf.data), buf.size
    else:
        raise ValueError('unexpected type for buf')

    context = parse_signature(sig) if isinstance(sig, str) else nullcontext(sig)
    with context as s:
        result = ctypes.c_void_p()
        status = _library.libhat_find_pattern(s.handle, data, size, ctypes.byref(result), align.value, hints.value)
        if status != 0:
            raise LibhatError(status)
        return result.value - data.value if result else None


def find_pattern_mod(sig: str | Signature, mod: Module, section: str, align: ScanAlignment = ScanAlignment.X1,
                     hints: ScanHint = ScanHint(0)) -> Address | None:
    context = parse_signature(sig) if isinstance(sig, str) else nullcontext(sig)
    with context as s:
        result = ctypes.c_void_p()
        status = _library.libhat_find_pattern_mod(s.handle, mod.handle, section.encode('utf-8'), ctypes.byref(result),
                                                  align.value, hints.value)
        if status != 0:
            raise LibhatError(status)
        return Address(result.value)


def is_readable(span: Span) -> bool:
    return _library.libhat_is_readable(span.data, span.size)


def is_writable(span: Span) -> bool:
    return _library.libhat_is_writable(span.data, span.size)


def is_executable(span: Span) -> bool:
    return _library.libhat_is_executable(span.data, span.size)


def get_process_module() -> Module:
    return Module(ctypes.cast(_library.libhat_get_process_module(), ctypes.c_void_p))


def get_module(name: str) -> Module | None:
    handle = ctypes.cast(_library.libhat_get_module(name.encode('utf-8')), ctypes.c_void_p)
    return Module(handle) if handle else None


def module_at(address: int) -> Module | None:
    handle = ctypes.cast(_library.libhat_module_at(address), ctypes.c_void_p)
    return Module(handle) if handle else None
