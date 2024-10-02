#include <libhat/System.hpp>

namespace hat {

    system_info::system_info() {}

    const system_info_impl system_info_impl::instance{};
    const system_info_impl& get_system() {
        return system_info_impl::instance;
    }
}
