#include <libhat/defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/system.hpp>
#include <Windows.h>

namespace hat {

    system_info::system_info() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        this->page_size = sysInfo.dwPageSize;
    }
}

#endif
