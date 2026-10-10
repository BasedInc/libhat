#include <libhat/defines.hpp>
#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)

#include <libhat/system.hpp>

#if defined(LIBHAT_WINDOWS) || defined(LIBHAT_MAC)
namespace hat {
    system_info_arm::system_info_arm() {
        this->extensions.neon = true;
    }
}
#endif

#if defined(LIBHAT_LINUX)

#include <asm/hwcap.h>
#include <sys/auxv.h>

namespace hat {
    system_info_arm::system_info_arm() {
#if defined(LIBHAT_ARM)
        unsigned long hwcap = getauxval(AT_HWCAP);
        this->extensions.neon = (hwcap & HWCAP_NEON) != 0;
#else // AARCH64
        unsigned long hwcap = getauxval(AT_HWCAP);
        this->extensions.neon = (hwcap & HWCAP_ASIMD) != 0;
#endif
    }
}
#endif

#endif
