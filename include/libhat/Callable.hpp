#pragma once

#include <new>
#include <utility>

#include "Concepts.hpp"
#include "Defines.hpp"

namespace hat::detail {

    template<typename Func>
    struct wrapper_util;

    template<typename Ret, typename... Args>
    struct wrapper_util<Ret(Args...)> {
        using function_type = Ret(Args...);

        template<supplier<Ret> Capture>
        struct context {
            Ret operator()(Args&&... args) const {
                return original(std::forward<Args>(args)...);
            }

            Ret call() const {
                return captured();
            }

            void* return_address() const {
                return ret_addr;
            }

            context(function_type* original, Capture&& captured, void* ret_addr)
                : original(original), captured(captured), ret_addr(ret_addr) {}

            context(const context&) = delete;
            context(context&&) = delete;
        private:
            function_type*  original;
            Capture         captured;
            void*           ret_addr;
        };

        template<typename T>
        context(function_type*, T&&, void*) -> context<T>;

        template<auto wrapper, auto get_original>
        struct caller {
            using wrapper_type = decltype(wrapper);

            static Ret invoke(Args... args) {
                const auto original = get_original();
                const context ctx{
                    original,
                    [&]() { return original(std::forward<Args>(args)...); },
                    LIBHAT_RETURN_ADDRESS()
                };
                using ctx_t = decltype(ctx);

                constexpr bool invoke_with_args = std::is_invocable_v<wrapper_type, const ctx_t&, Args&&...>;

                if constexpr (invoke_with_args) {
                    // If the wrapper wants to receive the function arguments
                    static_assert(std::is_same_v<Ret, std::invoke_result_t<wrapper_type, const ctx_t&, Args&&...>>);
                    return wrapper(ctx, std::forward<Args>(args)...);
                } else {
                    // Otherwise don't bother
                    static_assert(std::is_same_v<Ret, std::invoke_result_t<wrapper_type, const ctx_t&>>);
                    return wrapper(ctx);
                }
            };
        };

        template<supplier<function_type*> Provider>
        struct provider_storage {
            static inline function_type* get_original() {
                return (*std::launder(reinterpret_cast<Provider*>(instance)))();
            }

            static inline void set_provider(Provider&& provider) {
                new (instance) Provider{std::move(provider)};
            }
        private:
            alignas(Provider) static inline std::byte instance[sizeof(Provider)]{};
        };
    };
}

namespace hat::experimental {

    template<
        detail::function           Fn,
        std::default_initializable Wrapper,
        detail::supplier<Fn*>      Wrapped
    > requires std::default_initializable<Wrapped>
    constexpr auto make_dynamic_wrapper(Wrapper&&, Wrapped&&) -> Fn* {
        return detail::wrapper_util<Fn>::template caller<Wrapper{}, Wrapped{}>::invoke;
    }

    template<
        detail::function           Fn,
        std::default_initializable Wrapper,
        detail::supplier<Fn*>      Wrapped
    > requires (!std::default_initializable<Wrapped>)
    auto make_dynamic_wrapper(Wrapper&&, Wrapped&& wrapped) -> Fn* {
        using util_t = detail::wrapper_util<Fn>;
        const auto provider = [wrapped = std::move(wrapped)]() -> Fn* {
            return wrapped();
        };

        using storage = typename util_t::template provider_storage<decltype(provider)>;
        storage::set_provider(std::move(provider));

        return util_t::template caller<Wrapper{}, storage::get_original>::invoke;
    }

    template<auto wrapped, detail::function Fn = std::remove_pointer_t<decltype(wrapped)>>
    constexpr auto make_static_wrapper(std::default_initializable auto&& wrapper) {
        return make_dynamic_wrapper<Fn>(std::move(wrapper), []() { return wrapped; });
    }

    template<detail::function Fn>
    auto make_static_wrapper(std::default_initializable auto&& wrapper, Fn* wrapped) {
        return make_dynamic_wrapper<Fn>(std::move(wrapper), [=]() { return wrapped; });
    }

    template<typename Ret, typename... Args>
    auto make_static_wrapper(std::default_initializable auto&& wrapper, Ret(*wrapped)(Args...)) {
        return make_static_wrapper<Ret(Args...)>(std::move(wrapper), wrapped);
    }
}
