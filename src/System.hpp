#pragma once

#include <libhat/Defines.hpp>

#if defined(LIBHAT_X86)
#include "arch/x86/System.hpp"
#elif defined(LIBHAT_ARM)
#include "arch/arm/System.hpp"
#endif

namespace hat {

    const system_info& get_system();
}
