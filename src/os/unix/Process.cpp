#include <libhat/defines.hpp>
#ifdef LIBHAT_UNIX

#include <libhat/process.hpp>

#include <dlfcn.h>

#include <cstdlib>

namespace hat::process {

    hat::process::module get_process_module() {
        const auto module = get_module({});
        if (!module) {
            std::abort();
        }
        return *module;
    }
}

#endif
