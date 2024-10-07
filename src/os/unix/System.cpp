#include <libhat/Defines.hpp>
#ifdef LIBHAT_UNIX

#include <libhat/System.hpp>
#include <unistd.h>

namespace hat {

    system_info::system_info() {
        this->page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    }
}

#endif
