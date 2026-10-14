#include <boost/ut.hpp>

#include <cstddef>
#include <vector>

#include "ECS/Core/Entity/EntityList.hpp"
#include "ECS/Core/Entity/EntityManager.hpp"

using namespace boost::ut;

namespace {
    std::vector<ECS::Entity> collect(const ECS::EntityList& list) {
        std::vector<ECS::Entity> entities;
        for (ECS::Entity entity : list) {
            entities.push_back(entity);
        }
        return entities;
    }
}

namespace {
void test_list_iterates_live_entities_in_order() {
    "EntityList iterates the live entities in ascending index order"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity first = manager.create();
        const ECS::Entity second = manager.create();
        const ECS::EntityList list(manager);

        expect(collect(list) == std::vector<ECS::Entity>{first, second});
        expect(list.size() == 2_ul);
        expect(!list.empty());
    };
}

void test_list_of_empty_manager_is_empty() {
    "EntityList of an empty manager is empty"_test = [] {
        const ECS::EntityManager manager;
        const ECS::EntityList list(manager);

        expect(collect(list).empty());
        expect(list.size() == 0_ul);
        expect(list.empty());
        expect(list.begin() == list.end());
    };
}

void test_list_stays_up_to_date_without_being_recreated() {
    "a kept EntityList sees later creates and destroys"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity first = manager.create();
        const ECS::EntityList list(manager);
        expect(collect(list) == std::vector<ECS::Entity>{first});

        const ECS::Entity second = manager.create();
        manager.destroy(first);

        expect(collect(list) == std::vector<ECS::Entity>{second});
        expect(list.size() == 1_ul);
    };
}

void test_list_ignores_reserved_identities() {
    "EntityList ignores reserved identities until they are committed"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity reserved = manager.reserveIdentity();
        const ECS::EntityList list(manager);
        expect(list.empty());
        expect(collect(list).empty());

        manager.commit(reserved);

        expect(collect(list) == std::vector<ECS::Entity>{reserved});
    };
}

void test_loop_finishes_on_its_snapshot_when_entities_are_added() {
    "a loop does not see entities created during it, the next loop does"_test = [] {
        ECS::EntityManager manager;
        manager.create();
        manager.create();
        const ECS::EntityList list(manager);

        std::size_t visited = 0;
        for ([[maybe_unused]] ECS::Entity entity : list) {
            manager.create();
            ++visited;
        }

        expect(visited == 2_ul);
        expect(collect(list).size() == 4_ul);
    };
}

void test_loop_skips_entities_killed_during_it() {
    "a loop skips the entities killed while it runs"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity first = manager.create();
        const ECS::Entity second = manager.create();
        const ECS::Entity third = manager.create();
        const ECS::EntityList list(manager);

        std::vector<ECS::Entity> visited;
        for (ECS::Entity entity : list) {
            visited.push_back(entity);
            if (entity == first) {
                manager.destroy(second);
            }
        }

        expect(visited == std::vector<ECS::Entity>{first, third});
    };
}

void test_loop_can_kill_the_current_entity() {
    "a loop can destroy every entity it visits"_test = [] {
        ECS::EntityManager manager;
        manager.create();
        manager.create();
        const ECS::EntityList list(manager);

        std::size_t visited = 0;
        for (ECS::Entity entity : list) {
            manager.destroy(entity);
            ++visited;
        }

        expect(visited == 2_ul);
        expect(list.empty());
        expect(collect(list).empty());
    };
}

void test_iterator_keeps_its_current_entity_after_a_destroy() {
    "an iterator already on an entity keeps it after a destroy; a new iteration does not list it"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity first = manager.create();
        const ECS::Entity second = manager.create();
        const ECS::EntityList list(manager);

        auto it = list.begin();          // takes the snapshot {first, second}
        manager.destroy(first);
        auto again = list.begin();       // new snapshot {second}

        expect(*it == first);            // already positioned before the destroy
        ++it;
        expect(*it == second);
        expect(*again == second);
    };
}

void test_post_increment_returns_previous_position() {
    "Iterator post-increment returns the previous position"_test = [] {
        ECS::EntityManager manager;
        const ECS::Entity first = manager.create();
        const ECS::Entity second = manager.create();
        const ECS::EntityList list(manager);

        auto it = list.begin();
        const auto previous = it++;

        expect(*previous == first);
        expect(*it == second);
        ++it;
        expect(it == list.end());
    };
}

void test_default_iterator_is_at_end() {
    "a default-constructed Iterator compares equal to the end"_test = [] {
        const ECS::EntityList::Iterator it;
        expect(it == ECS::EntityList::Sentinel{});
    };
}

void test_lists_of_two_managers_are_independent() {
    "EntityLists of two managers only see their own entities"_test = [] {
        ECS::EntityManager first;
        ECS::EntityManager second;
        const ECS::Entity entity = first.create();

        expect(collect(ECS::EntityList(first)) == std::vector<ECS::Entity>{entity});
        expect(collect(ECS::EntityList(second)).empty());
    };
}
}

void run_entity_list_tests() {
    test_list_iterates_live_entities_in_order();
    test_list_of_empty_manager_is_empty();
    test_list_stays_up_to_date_without_being_recreated();
    test_list_ignores_reserved_identities();
    test_loop_finishes_on_its_snapshot_when_entities_are_added();
    test_loop_skips_entities_killed_during_it();
    test_loop_can_kill_the_current_entity();
    test_iterator_keeps_its_current_entity_after_a_destroy();
    test_post_increment_returns_previous_position();
    test_default_iterator_is_at_end();
    test_lists_of_two_managers_are_independent();
}
