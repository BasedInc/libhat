#include <concepts>
#include <new>
#include <utility>

namespace hat::detail {

    template<typename Fn, typename Ret>
    concept supplier = requires(Fn&& fn) {
        { fn() } -> std::same_as<Ret>;
    };

    template<typename Fn>
    concept function = std::is_function_v<Fn>;

    template<typename Func>
    struct wrapper_util;

    template<typename Ret, typename... Args>
    struct wrapper_util<Ret(Args...)> {
        using function_type = Ret(Args...);

        template<supplier<Ret> Capture>
        struct context {
            Ret operator()(Args&& ... args) const {
                return original(std::forward<Args>(args)...);
            }

            Ret call() const {
                return captured();
            }

            context(function_type* original, Capture&& captured) : original(original), captured(captured) {}

        private:
            function_type* original;
            Capture captured;
        };

        template<typename T>
        context(function_type*, T&&) -> context<T>;

        template<typename Wrapper, auto get_original>
        struct caller {
            static Ret invoke(Args... args) {
                const context ctx{
                    get_original(),
                    [&]() { return get_original()(std::forward<decltype(args)>(args)...); }
                };
                using ctx_t = decltype(ctx);

                constexpr bool invoke_with_args = std::is_invocable_v<Wrapper, const ctx_t&, Args&& ...>;

                if constexpr (invoke_with_args) {
                    // If the wrapper wants to receive the function arguments
                    static_assert(std::is_same_v<Ret, std::invoke_result_t<Wrapper, const ctx_t&, Args&& ...>>);
                    return Wrapper{}(ctx, std::forward<decltype(args)>(args)...);
                } else {
                    // Otherwise don't bother
                    static_assert(std::is_same_v<Ret, std::invoke_result_t<Wrapper, const ctx_t&>>);
                    return Wrapper{}(ctx);
                }
            };
        };

        template<typename Provider>
        struct capturing_static_storage {
            alignas(Provider) static inline std::byte impl[sizeof(Provider)]{};

            static inline function_type* get_original() {
                return (*std::launder(reinterpret_cast<Provider*>(impl)))();
            }
        };
    };
}

namespace hat {

    template<detail::function Fn, std::default_initializable Wrapper>
    auto make_dynamic_wrapper(Wrapper, detail::supplier<Fn*> auto wrapped) -> Fn* {
        using util_t = detail::wrapper_util<Fn>;
        if constexpr (std::is_default_constructible_v<decltype(wrapped)>) {
            return util_t::template caller<Wrapper, decltype(wrapped){}>::invoke;
        } else {
            const auto provider = [wrapped = std::move(wrapped)]() -> Fn* {
                return wrapped();
            };

            using storage = util_t::template capturing_static_storage<decltype(provider)>;
            new (storage::impl) decltype(provider){std::move(provider)};

            return util_t::template caller<Wrapper, storage::get_original>::invoke;
        }
    }

    template<auto wrapped, detail::function Fn = std::remove_pointer_t<decltype(wrapped)>>
    auto make_static_wrapper(auto wrapper) {
        return make_dynamic_wrapper<Fn>(wrapper, []() { return wrapped; });
    }

    template<detail::function Fn>
    auto make_static_wrapper(auto wrapper, Fn* wrapped) {
        return make_dynamic_wrapper<Fn>(wrapper, [=]() { return wrapped; });
    }

    template<typename Ret, typename... Args>
    auto make_static_wrapper(auto wrapper, Ret(* wrapped)(Args...)) {
        return make_static_wrapper<Ret(Args...)>(wrapper, wrapped);
    }
}
