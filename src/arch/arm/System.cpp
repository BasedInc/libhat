#include <libhat/defines.hpp>
#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)

#include <libhat/system.hpp>

namespace hat {

#ifdef LIBHAT_WINDOWS
    system_info_arm::system_info_arm() {
        this->extensions.neon = true;
    }
#endif

}
#endif
