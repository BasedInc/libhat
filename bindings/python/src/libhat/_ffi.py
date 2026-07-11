import ctypes
from pathlib import Path

# TODO
_library = ctypes.CDLL(Path(__file__).parent / 'bin' / 'libhat_c')

# ------------------------------------------------------------------------
# C types
# ------------------------------------------------------------------------
c_uintptr = ctypes.c_uint64 if ctypes.sizeof(ctypes.c_void_p) == 8 else ctypes.c_uint32

# ------------------------------------------------------------------------
# Structures
# ------------------------------------------------------------------------
from .span import Span

# ------------------------------------------------------------------------
# Callback types
# ------------------------------------------------------------------------
ForEachSectionCallback = ctypes.CFUNCTYPE(
    ctypes.c_bool,
    ctypes.c_char_p,
    Span,
    ctypes.c_int,
    ctypes.c_void_p,
)

ForEachSegmentCallback = ctypes.CFUNCTYPE(
    ctypes.c_bool,
    Span,
    ctypes.c_int,
    ctypes.c_void_p,
)

# ------------------------------------------------------------------------
# API
# ------------------------------------------------------------------------

#
# const char* libhat_get_version();
#
_library.libhat_get_version.argtypes = []
_library.libhat_get_version.restype = ctypes.c_char_p

#
# int libhat_get_version_num();
#
_library.libhat_get_version_num.argtypes = []
_library.libhat_get_version_num.restype = ctypes.c_int

#
# const char* libhat_status_to_string(libhat_status status);
#
_library.libhat_status_to_string.argtypes = [ctypes.c_int]
_library.libhat_status_to_string.restype = ctypes.c_char_p

#
# libhat_status libhat_parse_signature(
#     const char*              signatureStr,
#     const libhat_signature** signatureOut
# );
#
_library.libhat_parse_signature.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p)]
_library.libhat_parse_signature.restype = ctypes.c_int

#
# libhat_status libhat_create_signature(
#     const char*              bytes,
#     const char*              mask,
#     size_t                   size,
#     const libhat_signature** signatureOut
# );
#
_library.libhat_create_signature.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t,
                                             ctypes.POINTER(ctypes.c_void_p)]
_library.libhat_create_signature.restype = ctypes.c_int

#
# const void* libhat_find_pattern(
#     const libhat_signature* signature,
#     const void*             buffer,
#     size_t                  size,
#     libhat_alignment        align,
#     libhat_hint             hints
# );
#
_library.libhat_find_pattern.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int, ctypes.c_int]
_library.libhat_find_pattern.restype = ctypes.c_void_p

#
# const void* libhat_find_pattern_mod(
#     const libhat_signature* signature,
#     const libhat_module*    module,
#     const char*             section,
#     libhat_alignment        align,
#     libhat_hint             hints
# );
#
_library.libhat_find_pattern_mod.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int,
                                             ctypes.c_int]
_library.libhat_find_pattern_mod.restype = ctypes.c_void_p

#
# uintptr_t libhat_module_address(const libhat_module* module);
#
_library.libhat_module_address.argtypes = [ctypes.c_void_p]
_library.libhat_module_address.restype = c_uintptr

#
# libhat_span libhat_module_get_data(const libhat_module* module);
#
_library.libhat_module_get_data.argtypes = [ctypes.c_void_p]
_library.libhat_module_get_data.restype = Span

#
# libhat_span libhat_module_get_executable_data(const libhat_module* module);
#
_library.libhat_module_get_executable_data.argtypes = [ctypes.c_void_p]
_library.libhat_module_get_executable_data.restype = Span

#
# libhat_span libhat_module_get_section_data(const libhat_module* module, const char* name);
#
_library.libhat_module_get_section_data.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
_library.libhat_module_get_section_data.restype = Span

#
# void libhat_module_for_each_section(
#     const libhat_module*       module,
#     libhat_for_each_section_cb callback,
#     void*                      user_data
# );
#
_library.libhat_module_for_each_section.argtypes = [ctypes.c_void_p, ForEachSectionCallback, ctypes.c_void_p]
_library.libhat_module_for_each_section.restype = None

#
# void libhat_module_for_each_segment(
#     const libhat_module*       module,
#     libhat_for_each_segment_cb callback,
#     void*                      user_data
# );
#
_library.libhat_module_for_each_segment.argtypes = [ctypes.c_void_p, ForEachSegmentCallback, ctypes.c_void_p]
_library.libhat_module_for_each_segment.restype = None

#
# bool libhat_is_readable(const void* data, size_t size);
#
_library.libhat_is_readable.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
_library.libhat_is_readable.restype = ctypes.c_bool

#
# bool libhat_is_writable(const void* data, size_t size);
#
_library.libhat_is_writable.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
_library.libhat_is_writable.restype = ctypes.c_bool

#
# bool libhat_is_executable(const void* data, size_t size);
#
_library.libhat_is_executable.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
_library.libhat_is_executable.restype = ctypes.c_bool

#
# const libhat_module* libhat_get_process_module();
#
_library.libhat_get_process_module.argtypes = []
_library.libhat_get_process_module.restype = ctypes.c_void_p

#
# const libhat_module* libhat_get_module(const char* name);
#
_library.libhat_get_module.argtypes = [ctypes.c_char_p]
_library.libhat_get_module.restype = ctypes.c_void_p

#
# const libhat_module* libhat_module_at(const void* address);
#
_library.libhat_module_at.argtypes = [ctypes.c_void_p]
_library.libhat_module_at.restype = ctypes.c_void_p

#
# void libhat_free(const void* object);
#
_library.libhat_free.argtypes = [ctypes.c_void_p]
_library.libhat_free.restype = None
