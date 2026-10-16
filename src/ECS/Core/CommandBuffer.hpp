#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ECS {
    /**
     * @brief Queue of deferred commands, applied in order at the end of a
     *        recording scope.
     *
     * While record() runs its callable, the buffer is recording: the owner
     * (the Registry) pushes commands instead of applying changes, so a View
     * being iterated is never reordered under it. When the outermost
     * record() returns or throws, recording stops and the queued commands
     * run in the order they were pushed.
     *
     * The buffer knows nothing about what a command does: commands are plain
     * callables that capture what they need.
     */
    class CommandBuffer {
        public:
            using Command = std::function<void()>;

            /** @brief True while a record() call is running. */
            [[nodiscard]] bool recording() const noexcept {
                return _depth > 0;
            }

            /** @brief Number of commands waiting for the end of the recording scope. */
            [[nodiscard]] std::size_t size() const noexcept {
                return _commands.size();
            }

            /**
             * @brief Runs callable while recording, then applies the queued commands.
             *
             * Nested calls join the outer scope: commands are only applied
             * when the outermost record() ends. Recording stops and the
             * commands are applied even if callable throws; the exception is
             * then rethrown (an exception from a command replaces it).
             */
            template<typename Callable>
            void record(Callable&& callable) {
                ++_depth;
                try {
                    std::forward<Callable>(callable)();
                } catch (...) {
                    endScope();
                    throw;
                }
                endScope();
            }

            /**
             * @brief Queues a command until the end of the recording scope.
             * @throws std::logic_error when the buffer is not recording: a
             *         command pushed outside record() would never run.
             */
            void push(Command command) {
                if (!recording()) {
                    throw std::logic_error("ECS::CommandBuffer: push outside of record()");
                }
                _commands.push_back(std::move(command));
            }

        private:
            void endScope() {
                if (--_depth == 0) {
                    flush();
                }
            }

            // Takes the queue first: a throwing command propagates and drops
            // the remaining ones instead of replaying them on the next scope.
            void flush() {
                std::vector<Command> commands = std::move(_commands);
                _commands.clear();
                for (Command& command : commands) {
                    command();
                }
            }

            std::vector<Command> _commands;
            std::size_t _depth{0};
    };
}
