#pragma once

#ifndef LIBHAT_MODULE
    #include <cstdint>
    #include <concepts>
#endif

#include "export.hpp"
#include "type_traits.hpp"

LIBHAT_EXPORT namespace hat {

    template<typename MemberType, typename Base>
    auto& member_at(Base* ptr, std::integral auto offset) noexcept {
        using Ret = hat::constness_as_t<MemberType, Base>;
        return *reinterpret_cast<Ret*>(reinterpret_cast<std::uintptr_t>(ptr) + offset);
    }
}
