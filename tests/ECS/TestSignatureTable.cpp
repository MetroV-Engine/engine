#include <boost/ut.hpp>

#include <cstddef>

#include "ECS/ComponentId.hpp"
#include "ECS/SignatureTable.hpp"

using namespace boost::ut;

namespace {
    void test_table_starts_one_word_wide() {
        "a new table gives each entity one word"_test = [] {
            const ECS::SignatureTable table;
            expect(table.wordsPerEntity() == std::size_t{1});
        };
    }

    void test_new_row_is_empty() {
        "a new row has every bit off"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);

            for (ECS::ComponentId id = 0; id < 64; ++id) {
                expect(!table.test(0, id));
            }
        };
    }

    void test_set_marks_component_present() {
        "set marks a component present"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.set(0, 4);

            expect(table.test(0, 4));
            expect(!table.test(0, 3));
            expect(!table.test(0, 5));
        };
    }

    void test_reset_removes_one_component() {
        "reset removes one component and leaves the others"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.set(0, 4);
            table.set(0, 9);
            table.reset(0, 4);

            expect(!table.test(0, 4));
            expect(table.test(0, 9));
        };
    }

    void test_clear_removes_every_component() {
        "clear removes every component of one entity"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(1);
            table.set(0, 1);
            table.set(0, 40);
            table.set(0, 63);
            table.set(1, 40);
            table.clear(0);

            expect(!table.test(0, 1));
            expect(!table.test(0, 40));
            expect(!table.test(0, 63));
            expect(table.test(1, 40));
        };
    }

    void test_entities_are_independent() {
        "two entities never affect each other"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(1);
            table.set(0, 7);

            expect(table.test(0, 7));
            expect(!table.test(1, 7));

            table.set(1, 7);
            table.reset(0, 7);

            expect(!table.test(0, 7));
            expect(table.test(1, 7));
        };
    }

    void test_ensure_entity_creates_skipped_rows() {
        "ensureEntity creates every row up to the index"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(10);

            for (std::size_t entity = 0; entity <= 10; ++entity) {
                expect(!table.test(entity, 0));
            }
            table.set(3, 5);
            table.set(10, 5);
            expect(table.test(3, 5));
            expect(table.test(10, 5));
            expect(!table.test(4, 5));
        };
    }

    void test_ensure_entity_keeps_existing_bits() {
        "ensureEntity keeps the bits of existing rows"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.set(0, 12);
            table.ensureEntity(500);
            table.ensureEntity(0);

            expect(table.test(0, 12));
            expect(!table.test(500, 12));
        };
    }

    void test_ensure_component_does_not_grow_when_it_fits() {
        "ensureComponent does not grow when the id fits"_test = [] {
            ECS::SignatureTable table;
            table.ensureComponent(0);
            table.ensureComponent(63);

            expect(table.wordsPerEntity() == std::size_t{1});
        };
    }

    void test_ensure_component_doubles_the_width() {
        "ensureComponent doubles the width when an id does not fit"_test = [] {
            ECS::SignatureTable table;

            table.ensureComponent(64);
            expect(table.wordsPerEntity() == std::size_t{2});

            table.ensureComponent(127);
            expect(table.wordsPerEntity() == std::size_t{2});

            table.ensureComponent(128);
            expect(table.wordsPerEntity() == std::size_t{4});

            table.ensureComponent(256);
            expect(table.wordsPerEntity() == std::size_t{8});
        };
    }

    void test_ensure_component_jumps_several_doublings() {
        "ensureComponent jumps several doublings at once"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.ensureComponent(1000);

            // 1000 / 64 = word 15, so 16 words are needed.
            expect(table.wordsPerEntity() == std::size_t{16});
            table.set(0, 1000);
            expect(table.test(0, 1000));
        };
    }

    void test_ids_at_word_boundaries() {
        "ids on both sides of a word boundary are distinct"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.ensureComponent(128);

            table.set(0, 63);
            table.set(0, 64);
            table.set(0, 127);
            table.set(0, 128);

            expect(table.test(0, 63));
            expect(table.test(0, 64));
            expect(table.test(0, 127));
            expect(table.test(0, 128));
            expect(!table.test(0, 62));
            expect(!table.test(0, 65));
            expect(!table.test(0, 126));
            expect(!table.test(0, 129));

            table.reset(0, 64);
            expect(table.test(0, 63));
            expect(!table.test(0, 64));
        };
    }

    void test_growth_keeps_every_entitys_bits() {
        "growing keeps exactly the bits every entity had"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(2);
            table.set(0, 0);
            table.set(0, 63);
            table.set(1, 5);
            // entity 2 owns nothing

            table.ensureComponent(300);

            expect(table.test(0, 0));
            expect(table.test(0, 63));
            expect(!table.test(0, 5));
            expect(table.test(1, 5));
            expect(!table.test(1, 0));
            expect(!table.test(1, 63));
            expect(!table.test(2, 0));
            expect(!table.test(2, 5));
            expect(!table.test(2, 63));

            for (std::size_t entity = 0; entity <= 2; ++entity) {
                for (ECS::ComponentId id = 64; id <= 300; ++id) {
                    expect(!table.test(entity, id));
                }
            }
        };
    }

    void test_rows_added_after_growth_are_empty_and_wide() {
        "a row added after growing is empty and as wide as the others"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.set(0, 3);
            table.ensureComponent(300);
            table.ensureEntity(1);

            expect(!table.test(1, 3));
            expect(!table.test(1, 300));
            table.set(1, 300);
            expect(table.test(1, 300));
            expect(!table.test(0, 300));
            expect(table.test(0, 3));
        };
    }

    void test_growth_with_no_entities() {
        "growing an empty table is safe"_test = [] {
            ECS::SignatureTable table;
            table.ensureComponent(200);
            expect(table.wordsPerEntity() == std::size_t{4});

            table.ensureEntity(0);
            table.set(0, 200);
            expect(table.test(0, 200));
        };
    }

    void test_clear_after_growth_clears_high_ids() {
        "clear after growing also clears high ids"_test = [] {
            ECS::SignatureTable table;
            table.ensureEntity(0);
            table.ensureComponent(300);
            table.set(0, 2);
            table.set(0, 300);
            table.clear(0);

            expect(!table.test(0, 2));
            expect(!table.test(0, 300));
        };
    }

    void test_test_is_false_for_unknown_id_or_entity() {
        "test returns false for an id too wide or an entity with no row"_test = [] {
            ECS::SignatureTable table;
            expect(!table.test(0, 0));

            table.ensureEntity(0);
            table.set(0, 1);

            expect(!table.test(0, 64));
            expect(!table.test(0, 100000));
            expect(!table.test(1, 1));
            expect(!table.test(100000, 1));
            expect(table.wordsPerEntity() == std::size_t{1});
        };
    }
}

void run_signature_table_tests() {
    test_table_starts_one_word_wide();
    test_new_row_is_empty();
    test_set_marks_component_present();
    test_reset_removes_one_component();
    test_clear_removes_every_component();
    test_entities_are_independent();
    test_ensure_entity_creates_skipped_rows();
    test_ensure_entity_keeps_existing_bits();
    test_ensure_component_does_not_grow_when_it_fits();
    test_ensure_component_doubles_the_width();
    test_ensure_component_jumps_several_doublings();
    test_ids_at_word_boundaries();
    test_growth_keeps_every_entitys_bits();
    test_rows_added_after_growth_are_empty_and_wide();
    test_growth_with_no_entities();
    test_clear_after_growth_clears_high_ids();
    test_test_is_false_for_unknown_id_or_entity();
}
