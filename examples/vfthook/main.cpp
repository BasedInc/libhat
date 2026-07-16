#include <libhat/memory_protector.hpp>

#include <bit>
#include <iostream>
#include <memory>

struct Interface {
    virtual void foo() = 0;
    virtual ~Interface() = default;
};

struct Impl : Interface {
    void foo() override {
        std::cout << "Impl::foo()" << std::endl;
    }
};

std::unique_ptr<Interface> create_interface() {
    return std::make_unique<Impl>();
}

int main() {
    const auto iface = create_interface();
    iface->foo(); // Prints "Impl::foo()"

    {
        uintptr_t* vftable = *reinterpret_cast<uintptr_t**>(iface.get());

        hat::memory_protector prot{std::bit_cast<uintptr_t>(vftable), sizeof(uintptr_t),
            hat::protection::Read | hat::protection::Write};

        vftable[0] = std::bit_cast<uintptr_t>(+[](Impl&) {
            std::cout << "fooHook()" << std::endl;
        });
    }

    iface->foo(); // Prints "fooHook()"
}
