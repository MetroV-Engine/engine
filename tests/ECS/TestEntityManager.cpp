#include <boost/ut.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "ECS/EntityManager.hpp"

using namespace boost::ut;

namespace ECS::TestAccess {
	struct EntityManagerAccess {
		static void setNextId(EntityManager& manager, std::size_t nextId) {
			manager._nextId = nextId;
		}

		static void setGeneration(
			EntityManager& manager,
			std::size_t index,
			EntityGeneration generation
		) {
			manager._alive.resize(index + 1, false);
			manager._generations.resize(index + 1, 0);
			manager._alive[index] = true;
			manager._generations[index] = generation;
		}
	};
}

void test_create_empty_manager() {
	"create returns the first live entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();

		expect(entity == ECS::Entity{0, 0});
		expect(manager.isAlive(entity));
	};
}

void test_create_assigns_sequential_indexes() {
	"create assigns sequential indexes"_test = [] {
		ECS::EntityManager manager;

		expect(manager.create().index() == std::size_t{0});
		expect(manager.create().index() == std::size_t{1});
		expect(manager.create().index() == std::size_t{2});
	};
}

void test_create_increases_size() {
	"create increases the live entity count"_test = [] {
		ECS::EntityManager manager;

		expect(manager.size() == std::size_t{0});
		manager.create();
		expect(manager.size() == std::size_t{1});
		manager.create();
		expect(manager.size() == std::size_t{2});
	};
}

void test_create_reuses_destroyed_index() {
	"create reuses a destroyed index"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		manager.destroy(first);

		expect(manager.create().index() == first.index());
	};
}

void test_create_reused_entity_has_incremented_generation() {
	"reused entities have an incremented generation"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		manager.destroy(first);
		const ECS::Entity second = manager.create();

		expect(second.generation() == ECS::EntityGeneration{1});
	};
}

void test_create_invalidates_old_handle_after_reuse() {
	"reusing an index invalidates the old handle"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		manager.destroy(first);
		const ECS::Entity second = manager.create();

		expect(!manager.isAlive(first));
		expect(manager.isAlive(second));
	};
}

void test_create_reuses_free_indexes_in_lifo_order() {
	"create reuses free indexes in LIFO order"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();
		manager.destroy(first);
		manager.destroy(second);

		expect(manager.create().index() == second.index());
		expect(manager.create().index() == first.index());
	};
}

void test_create_does_not_reuse_live_indexes() {
	"create does not reuse live indexes"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();

		expect(first.index() != second.index());
	};
}

void test_destroy_live_entity() {
	"destroy marks a live entity as dead"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();

		manager.destroy(entity);

		expect(!manager.isAlive(entity));
		expect(manager.size() == std::size_t{0});
	};
}

void test_destroy_adds_index_to_reuse_pool() {
	"destroy returns the index to the reuse pool"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);

		expect(manager.create().index() == entity.index());
	};
}

void test_destroy_rejects_default_entity() {
	"destroy rejects a default entity in an empty manager"_test = [] {
		ECS::EntityManager manager;

		expect(throws<std::invalid_argument>([&] {
			manager.destroy(ECS::Entity{});
		}));
	};
}

void test_destroy_rejects_unallocated_entity() {
	"destroy rejects an unallocated entity"_test = [] {
		ECS::EntityManager manager;

		expect(throws<std::invalid_argument>([&] {
			manager.destroy(ECS::Entity{42});
		}));
	};
}

void test_destroy_rejects_dead_entity() {
	"destroy rejects an already dead entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);

		expect(throws<std::invalid_argument>([&] {
			manager.destroy(entity);
		}));
	};
}

void test_destroy_rejects_stale_handle() {
	"destroy rejects a stale handle after reuse"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity oldEntity = manager.create();
		manager.destroy(oldEntity);
		manager.create();

		expect(throws<std::invalid_argument>([&] {
			manager.destroy(oldEntity);
		}));
	};
}

void test_destroy_rejects_handle_with_wrong_generation() {
	"destroy rejects a handle with the wrong generation"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		const ECS::Entity wrongGeneration{
			entity.index(),
			ECS::EntityGeneration{1}
		};

		expect(throws<std::invalid_argument>([&] {
			manager.destroy(wrongGeneration);
		}));
	};
}

void test_destroy_rejects_handle_from_another_manager() {
	"destroy rejects a handle from another manager"_test = [] {
		ECS::EntityManager firstManager;
		ECS::EntityManager secondManager;
		const ECS::Entity entity = firstManager.create();

		expect(throws<std::invalid_argument>([&] {
			secondManager.destroy(entity);
		}));
	};
}

void test_is_alive_returns_false_for_empty_manager() {
	"isAlive is false for an empty manager"_test = [] {
		ECS::EntityManager manager;

		expect(!manager.isAlive(ECS::Entity{0}));
	};
}

void test_is_alive_returns_true_for_created_entity() {
	"isAlive is true for a created entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();

		expect(manager.isAlive(entity));
	};
}

void test_is_alive_returns_false_after_destroy() {
	"isAlive is false after destroy"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);

		expect(!manager.isAlive(entity));
	};
}

void test_is_alive_rejects_wrong_index() {
	"isAlive rejects a wrong index"_test = [] {
		ECS::EntityManager manager;
		manager.create();

		expect(!manager.isAlive(ECS::Entity{1}));
	};
}

void test_is_alive_rejects_wrong_generation() {
	"isAlive rejects a wrong generation"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();

		expect(!manager.isAlive(ECS::Entity{
			entity.index(),
			ECS::EntityGeneration{1}
		}));
	};
}

void test_is_alive_rejects_stale_handle_after_reuse() {
	"isAlive rejects a stale handle after reuse"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity oldEntity = manager.create();
		manager.destroy(oldEntity);
		manager.create();

		expect(!manager.isAlive(oldEntity));
	};
}

void test_is_alive_rejects_entity_from_another_manager() {
	"isAlive rejects an entity from another manager"_test = [] {
		ECS::EntityManager firstManager;
		ECS::EntityManager secondManager;
		const ECS::Entity entity = firstManager.create();

		expect(!secondManager.isAlive(entity));
	};
}

void test_entity_from_index_returns_current_live_handle() {
	"entityFromIndex returns the current live handle"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();

		expect(manager.entityFromIndex(entity.index()) == entity);
	};
}

void test_entity_from_index_returns_current_generation_after_destroy() {
	"entityFromIndex returns the current generation after destroy"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);

		const ECS::Entity current = manager.entityFromIndex(entity.index());
		expect(current.generation() == ECS::EntityGeneration{1});
		expect(!manager.isAlive(current));
	};
}

void test_entity_from_index_returns_default_generation_for_unallocated_index() {
	"entityFromIndex uses generation zero for an unknown index"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.entityFromIndex(42);

		expect(entity == ECS::Entity{42, 0});
	};
}

void test_entity_from_index_does_not_make_entity_alive() {
	"entityFromIndex does not create an entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.entityFromIndex(42);

		expect(!manager.isAlive(entity));
		expect(manager.size() == std::size_t{0});
	};
}

void test_entity_from_index_returns_same_handle_after_reuse() {
	"entityFromIndex returns the new handle after reuse"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity oldEntity = manager.create();
		manager.destroy(oldEntity);
		const ECS::Entity newEntity = manager.create();

		expect(manager.entityFromIndex(newEntity.index()) == newEntity);
	};
}

void test_size_is_zero_for_empty_manager() {
	"size is zero for an empty manager"_test = [] {
		const ECS::EntityManager manager;

		expect(manager.size() == std::size_t{0});
	};
}

void test_size_counts_live_entities() {
	"size counts live entities"_test = [] {
		ECS::EntityManager manager;
		manager.create();
		manager.create();
		manager.create();

		expect(manager.size() == std::size_t{3});
	};
}

void test_size_decreases_after_destroy() {
	"size decreases after destroy"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.create();
		manager.destroy(entity);

		expect(manager.size() == std::size_t{1});
	};
}

void test_size_is_unchanged_when_reusing_an_index() {
	"size stays correct when reusing an index"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);
		manager.create();

		expect(manager.size() == std::size_t{1});
	};
}

void test_size_counts_multiple_destroyed_entities() {
	"size counts live entities after multiple destroys"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();
		manager.create();
		manager.destroy(first);
		manager.destroy(second);

		expect(manager.size() == std::size_t{1});
	};
}

void test_get_all_is_empty_for_empty_manager() {
	"getAll is empty for an empty manager"_test = [] {
		const ECS::EntityManager manager;

		expect(manager.getAll().empty());
	};
}

void test_get_all_returns_all_live_entities() {
	"getAll returns all live entities"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{2});
		expect(entities[0] == first);
		expect(entities[1] == second);
	};
}

void test_get_all_returns_entities_in_ascending_index_order() {
	"getAll returns entities in ascending index order"_test = [] {
		ECS::EntityManager manager;
		manager.create();
		manager.create();
		manager.create();

		const auto entities = manager.getAll();
		expect(entities[0].index() < entities[1].index());
		expect(entities[1].index() < entities[2].index());
	};
}

void test_get_all_excludes_destroyed_entities() {
	"getAll excludes destroyed entities"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();
		manager.destroy(first);

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{1});
		expect(entities[0] == second);
	};
}

void test_get_all_preserves_generations() {
	"getAll preserves current generations"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		manager.destroy(first);
		const ECS::Entity second = manager.create();

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{1});
		expect(entities[0] == second);
	};
}

void test_get_all_returns_only_current_handles() {
	"getAll excludes stale handles"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity oldEntity = manager.create();
		manager.destroy(oldEntity);
		const ECS::Entity newEntity = manager.create();

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{1});
		expect(entities[0].index() == oldEntity.index());
		expect(entities[0].generation() != oldEntity.generation());
		expect(entities[0] == newEntity);
	};
}

void test_get_all_is_correct_after_destroying_first_entity() {
	"getAll remains ordered after destroying the first entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();
		const ECS::Entity third = manager.create();
		manager.destroy(first);

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{2});
		expect(entities[0] == second);
		expect(entities[1] == third);
	};
}

void test_get_all_is_correct_after_destroying_last_entity() {
	"getAll remains ordered after destroying the last entity"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		const ECS::Entity second = manager.create();
		const ECS::Entity third = manager.create();
		manager.destroy(third);

		const auto entities = manager.getAll();
		expect(entities.size() == std::size_t{2});
		expect(entities[0] == first);
		expect(entities[1] == second);
	};
}

void test_get_all_result_is_independent_from_later_manager_changes() {
	"getAll returns an independent snapshot"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity first = manager.create();
		auto entities = manager.getAll();
		manager.destroy(first);

		expect(entities.size() == std::size_t{1});
		expect(entities[0] == first);
	};
}

void test_reserve_does_not_change_size() {
	"reserve does not change size"_test = [] {
		ECS::EntityManager manager;
		manager.reserve(10);

		expect(manager.size() == std::size_t{0});
	};
}

void test_reserve_does_not_create_entities() {
	"reserve does not create entities"_test = [] {
		ECS::EntityManager manager;
		manager.reserve(10);

		expect(manager.getAll().empty());
	};
}

void test_reserve_supports_creation_after_reservation() {
	"reserve supports later creation"_test = [] {
		ECS::EntityManager manager;
		manager.reserve(3);
		const ECS::Entity entity = manager.create();

		expect(entity == ECS::Entity{0, 0});
		expect(manager.isAlive(entity));
	};
}

void test_reserve_with_zero_does_not_change_behavior() {
	"reserve zero preserves normal behavior"_test = [] {
		ECS::EntityManager manager;
		manager.reserve(0);
		const ECS::Entity entity = manager.create();
		manager.destroy(entity);

		expect(manager.size() == std::size_t{0});
		expect(manager.create().index() == entity.index());
	};
}

void test_multiple_reserve_calls_preserve_state() {
	"multiple reserve calls preserve manager state"_test = [] {
		ECS::EntityManager manager;
		const ECS::Entity entity = manager.create();
		manager.reserve(2);
		manager.reserve(10);

		expect(manager.size() == std::size_t{1});
		expect(manager.isAlive(entity));
	};
}

void test_create_throws_when_entity_id_overflows() {
	"create throws when the entity id overflows"_test = [] {
		ECS::EntityManager manager;
		ECS::TestAccess::EntityManagerAccess::setNextId(
			manager,
			std::numeric_limits<std::size_t>::max()
		);

		expect(throws<std::overflow_error>([&] {
			manager.create();
		}));
	};
}

void test_destroy_throws_when_generation_overflows() {
	"destroy throws when the generation overflows"_test = [] {
		ECS::EntityManager manager;
		constexpr std::size_t index = 0;
		constexpr ECS::EntityGeneration generation =
			std::numeric_limits<ECS::EntityGeneration>::max();
		ECS::TestAccess::EntityManagerAccess::setGeneration(
			manager,
			index,
			generation
		);

		expect(throws<std::overflow_error>([&] {
			manager.destroy(ECS::Entity{index, generation});
		}));
	};
}

void run_entity_manager_tests()
{
	test_create_empty_manager();
	test_create_assigns_sequential_indexes();
	test_create_increases_size();
	test_create_reuses_destroyed_index();
	test_create_reused_entity_has_incremented_generation();
	test_create_invalidates_old_handle_after_reuse();
	test_create_reuses_free_indexes_in_lifo_order();
	test_create_does_not_reuse_live_indexes();
	test_destroy_live_entity();
	test_destroy_adds_index_to_reuse_pool();
	test_destroy_rejects_default_entity();
	test_destroy_rejects_unallocated_entity();
	test_destroy_rejects_dead_entity();
	test_destroy_rejects_stale_handle();
	test_destroy_rejects_handle_with_wrong_generation();
	test_destroy_rejects_handle_from_another_manager();
	test_is_alive_returns_false_for_empty_manager();
	test_is_alive_returns_true_for_created_entity();
	test_is_alive_returns_false_after_destroy();
	test_is_alive_rejects_wrong_index();
	test_is_alive_rejects_wrong_generation();
	test_is_alive_rejects_stale_handle_after_reuse();
	test_is_alive_rejects_entity_from_another_manager();
	test_entity_from_index_returns_current_live_handle();
	test_entity_from_index_returns_current_generation_after_destroy();
	test_entity_from_index_returns_default_generation_for_unallocated_index();
	test_entity_from_index_does_not_make_entity_alive();
	test_entity_from_index_returns_same_handle_after_reuse();
	test_size_is_zero_for_empty_manager();
	test_size_counts_live_entities();
	test_size_decreases_after_destroy();
	test_size_is_unchanged_when_reusing_an_index();
	test_size_counts_multiple_destroyed_entities();
	test_get_all_is_empty_for_empty_manager();
	test_get_all_returns_all_live_entities();
	test_get_all_returns_entities_in_ascending_index_order();
	test_get_all_excludes_destroyed_entities();
	test_get_all_preserves_generations();
	test_get_all_returns_only_current_handles();
	test_get_all_is_correct_after_destroying_first_entity();
	test_get_all_is_correct_after_destroying_last_entity();
	test_get_all_result_is_independent_from_later_manager_changes();
	test_reserve_does_not_change_size();
	test_reserve_does_not_create_entities();
	test_reserve_supports_creation_after_reservation();
	test_reserve_with_zero_does_not_change_behavior();
	test_multiple_reserve_calls_preserve_state();
	test_create_throws_when_entity_id_overflows();
	test_destroy_throws_when_generation_overflows();
}