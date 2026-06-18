#include <libhat/defines.hpp>
#if defined(LIBHAT_ARM) || defined(LIBHAT_AARCH64)

#include <libhat/system.hpp>

namespace hat {

#if defined(LIBHAT_WINDOWS) || defined(LIBHAT_MAC)
    system_info_arm::system_info_arm() {
        this->extensions.neon = true;
    }
#endif

}
#endif
