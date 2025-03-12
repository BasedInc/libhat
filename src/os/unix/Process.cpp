#include <libhat/defines.hpp>
#ifdef LIBHAT_UNIX

#include <libhat/process.hpp>

#include <dlfcn.h>

namespace hat::process {

    hat::process::module get_process_module() {
        return module{reinterpret_cast<uintptr_t>(dlopen(nullptr, RTLD_LAZY))};
    }
}

#endif
