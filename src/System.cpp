#include <libhat/System.hpp>

namespace hat {

    const system_info_impl system_info_impl::instance{};
    const system_info_impl& get_system() {
        return system_info_impl::instance;
    }
}
