#pragma once

#include <concepts>

#include "Traits.hpp"

namespace hat {

    template<typename MemberType, typename Base>
    auto& member_at(Base* ptr, std::integral auto offset) {
        using Ret = hat::constness_as_t<MemberType, Base>;
        return *reinterpret_cast<Ret*>(reinterpret_cast<uintptr_t>(ptr) + offset);
    }
}
