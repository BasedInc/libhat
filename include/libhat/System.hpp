#pragma once

#include "Defines.hpp"

#if defined(LIBHAT_X86)
#include "../../src/arch/x86/System.hpp"
#elif defined(LIBHAT_ARM)
#include "../../src/arch/arm/System.hpp"
#endif

namespace hat {

    const system_info& get_system();
}
