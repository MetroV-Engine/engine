#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "ComponentId.hpp"

namespace ECS {
    /**
     * @brief Growable table of membership bits: which component types each
     *        entity currently owns.
     *
     * Every entity gets a row of wordsPerEntity() 64-bit words, and all rows
     * sit back to back in one buffer: entity e owns the words
     * [e * W, e * W + W). Bit (id % 64) of word (id / 64) in that row says
     * whether the entity owns the component type with that id.
     *
     * The row width is the same for every entity. It starts at one word (64
     * component types) and doubles whenever a component id does not fit, so
     * there is no fixed limit on the number of component types. Growing
     * copies the bits only; component data lives in its ComponentStorage and
     * is never touched here.
     *
     * The table knows nothing about entity liveness: Registry decides when a
     * row is created, set or cleared.
     */
    class SignatureTable {
        public:
            /** @brief Guarantees a row exists for this entity index. New rows are empty. */
            void ensureEntity(std::size_t entity) {
                if (entity < _entityCount) {
                    return;
                }
                _entityCount = entity + 1;
                _words.resize(_entityCount * _wordsPerEntity, 0);
            }

            /**
             * @brief Guarantees every row is wide enough for this component id.
             *
             * The only place the table grows. Doubles the row width until id
             * fits, then moves each entity's words to the start of its wider
             * row; the added words are zero.
             */
            void ensureComponent(ComponentId id) {
                const std::size_t needed = wordOf(id) + 1;
                if (needed <= _wordsPerEntity) {
                    return;
                }
                std::size_t grown = _wordsPerEntity;
                while (grown < needed) {
                    grown *= 2;
                }
                std::vector<std::uint64_t> words(_entityCount * grown, 0);
                for (std::size_t entity = 0; entity < _entityCount; ++entity) {
                    std::copy_n(_words.begin() + static_cast<std::ptrdiff_t>(entity * _wordsPerEntity),
                                _wordsPerEntity,
                                words.begin() + static_cast<std::ptrdiff_t>(entity * grown));
                }
                _words = std::move(words);
                _wordsPerEntity = grown;
            }

            /** @brief Marks the component as owned. The row must exist and id must fit. */
            void set(std::size_t entity, ComponentId id) noexcept {
                _words[entity * _wordsPerEntity + wordOf(id)] |= bitOf(id);
            }

            /** @brief Marks the component as not owned. The row must exist and id must fit. */
            void reset(std::size_t entity, ComponentId id) noexcept {
                _words[entity * _wordsPerEntity + wordOf(id)] &= ~bitOf(id);
            }

            /** @brief Marks every component as not owned. The row must exist. */
            void clear(std::size_t entity) noexcept {
                std::fill_n(_words.begin() + static_cast<std::ptrdiff_t>(entity * _wordsPerEntity),
                            _wordsPerEntity, std::uint64_t{0});
            }

            /**
             * @brief Checks whether the entity owns the component.
             * @return False when the entity has no row or id is wider than the
             *         rows: such a component can't have been added here.
             */
            [[nodiscard]] bool test(std::size_t entity, ComponentId id) const noexcept {
                const std::size_t word = wordOf(id);
                if (entity >= _entityCount || word >= _wordsPerEntity) {
                    return false;
                }
                return (_words[entity * _wordsPerEntity + word] & bitOf(id)) != 0;
            }

            /** @brief Number of 64-bit words in each entity's row. */
            [[nodiscard]] std::size_t wordsPerEntity() const noexcept {
                return _wordsPerEntity;
            }

        private:
            static constexpr std::size_t BitsPerWord = 64;

            static constexpr std::size_t wordOf(ComponentId id) noexcept {
                return static_cast<std::size_t>(id) / BitsPerWord;
            }

            static constexpr std::uint64_t bitOf(ComponentId id) noexcept {
                return std::uint64_t{1} << (static_cast<std::size_t>(id) % BitsPerWord);
            }

            std::vector<std::uint64_t> _words;
            std::size_t _wordsPerEntity{1};
            std::size_t _entityCount{0};
    };
}
