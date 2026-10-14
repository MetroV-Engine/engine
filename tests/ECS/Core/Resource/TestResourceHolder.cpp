#include <boost/ut.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "ECS/Core/Resource/ResourceHolder.hpp"

using namespace boost::ut;

namespace {
    struct Settings {
        std::string name;
        int volume{0};

        Settings() = default;
        Settings(std::string n, int v) : name(std::move(n)), volume(v) {}
    };

    struct DestructionCounter {
        int* destroyed;

        explicit DestructionCounter(int* counter) : destroyed(counter) {}
        ~DestructionCounter() { ++*destroyed; }
    };
}

namespace {
void test_holder_forwards_constructor_arguments() {
    "ResourceHolder forwards its arguments to the resource constructor"_test = [] {
        ECS::ResourceHolder<Settings> holder("Metro", 3);

        expect(holder.value().name == "Metro");
        expect(holder.value().volume == 3_i);
    };
}

void test_holder_default_constructs_without_arguments() {
    "ResourceHolder default-constructs the resource without arguments"_test = [] {
        ECS::ResourceHolder<Settings> holder;

        expect(holder.value().name.empty());
        expect(holder.value().volume == 0_i);
    };
}

void test_holder_value_is_mutable() {
    "ResourceHolder::value returns a mutable reference"_test = [] {
        ECS::ResourceHolder<Settings> holder;
        holder.value().volume = 9;

        expect(holder.value().volume == 9_i);
    };
}

void test_holder_value_is_const_accessible() {
    "ResourceHolder::value is available on a const holder"_test = [] {
        const ECS::ResourceHolder<Settings> holder("Const", 1);

        static_assert(std::is_same_v<decltype(holder.value()), const Settings&>);
        expect(holder.value().name == "Const");
    };
}

void test_holder_stores_a_non_owning_pointer() {
    "ResourceHolder stores a raw pointer without owning its target"_test = [] {
        Settings external{"External", 5};
        {
            ECS::ResourceHolder<Settings*> holder(&external);
            holder.value()->volume = 6;
        }

        expect(external.volume == 6_i);
    };
}

void test_holder_destroys_value_through_interface() {
    "deleting a holder through IResourceHolder destroys the resource"_test = [] {
        int destroyed = 0;
        std::unique_ptr<ECS::IResourceHolder> holder =
            std::make_unique<ECS::ResourceHolder<DestructionCounter>>(&destroyed);

        holder.reset();

        expect(destroyed == 1_i);
    };
}
}

void run_resource_holder_tests() {
    test_holder_forwards_constructor_arguments();
    test_holder_default_constructs_without_arguments();
    test_holder_value_is_mutable();
    test_holder_value_is_const_accessible();
    test_holder_stores_a_non_owning_pointer();
    test_holder_destroys_value_through_interface();
}
