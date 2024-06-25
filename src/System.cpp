#include <libhat/System.hpp>

namespace hat {

    const system_info system_info::instance{};
    const system_info& get_system() {
        return system_info::instance;
    }
}
