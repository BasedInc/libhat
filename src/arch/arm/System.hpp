#pragma once

namespace hat {

    typedef struct system_info_arm {
        system_info_arm(const system_info_arm&) = delete;
        system_info_arm& operator=(const system_info_arm&) = delete;
    private:
        system_info_arm() = default;
        friend const system_info_arm& get_system();
        static const system_info_arm instance;
    } system_info;
}
