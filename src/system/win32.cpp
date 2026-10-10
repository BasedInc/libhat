#include <libhat/defines.hpp>
#ifdef LIBHAT_WINDOWS

#include <libhat/system.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace hat {

    system_info::system_info() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        this->page_size = sysInfo.dwPageSize;
    }
}

#endif
