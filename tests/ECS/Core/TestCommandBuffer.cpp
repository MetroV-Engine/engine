#include <boost/ut.hpp>

#include <stdexcept>
#include <vector>

#include "ECS/Core/CommandBuffer.hpp"

using namespace boost::ut;

namespace {
void test_buffer_is_not_recording_by_default() {
    "a new CommandBuffer is not recording and holds no command"_test = [] {
        const ECS::CommandBuffer buffer;

        expect(!buffer.recording());
        expect(buffer.size() == 0_ul);
    };
}

void test_record_runs_callable_while_recording() {
    "record runs its callable while recording"_test = [] {
        ECS::CommandBuffer buffer;
        bool recordingInside = false;

        buffer.record([&] { recordingInside = buffer.recording(); });

        expect(recordingInside);
        expect(!buffer.recording());
    };
}

void test_commands_run_after_callable_in_push_order() {
    "pushed commands run after the callable, in push order"_test = [] {
        ECS::CommandBuffer buffer;
        std::vector<int> order;

        buffer.record([&] {
            buffer.push([&] { order.push_back(1); });
            buffer.push([&] { order.push_back(2); });
            order.push_back(0);
        });

        expect(order == std::vector<int>{0, 1, 2});
        expect(buffer.size() == 0_ul);
    };
}

void test_push_queues_until_end_of_scope() {
    "push queues the command until the end of the scope"_test = [] {
        ECS::CommandBuffer buffer;
        bool ran = false;
        std::size_t sizeInside = 0;

        buffer.record([&] {
            buffer.push([&] { ran = true; });
            sizeInside = buffer.size();
            expect(!ran);
        });

        expect(sizeInside == 1_ul);
        expect(ran);
    };
}

void test_push_outside_record_throws() {
    "push outside of record throws and queues nothing"_test = [] {
        ECS::CommandBuffer buffer;

        expect(throws<std::logic_error>([&] { buffer.push([] {}); }));
        expect(buffer.size() == 0_ul);
    };
}

void test_record_without_commands_is_a_no_op() {
    "record with no pushed command leaves the buffer empty"_test = [] {
        ECS::CommandBuffer buffer;
        int calls = 0;

        buffer.record([&] { ++calls; });

        expect(calls == 1_i);
        expect(buffer.size() == 0_ul);
        expect(!buffer.recording());
    };
}

void test_throwing_callable_still_flushes_and_rethrows() {
    "a throwing callable stops recording, flushes, then rethrows"_test = [] {
        ECS::CommandBuffer buffer;
        bool ran = false;

        expect(throws<std::runtime_error>([&] {
            buffer.record([&] {
                buffer.push([&] { ran = true; });
                throw std::runtime_error("callable failed");
            });
        }));

        expect(ran);
        expect(!buffer.recording());
        expect(buffer.size() == 0_ul);
    };
}

void test_throwing_command_propagates_and_drops_the_rest() {
    "a throwing command propagates and the remaining commands are dropped"_test = [] {
        ECS::CommandBuffer buffer;
        bool laterRan = false;

        expect(throws<std::runtime_error>([&] {
            buffer.record([&] {
                buffer.push([] { throw std::runtime_error("command failed"); });
                buffer.push([&] { laterRan = true; });
            });
        }));

        expect(!laterRan);
        expect(!buffer.recording());
        expect(buffer.size() == 0_ul);

        bool nextRan = false;
        buffer.record([&] { buffer.push([&] { nextRan = true; }); });
        expect(nextRan);
        expect(!laterRan);
    };
}

void test_nested_record_joins_the_outer_scope() {
    "a nested record joins the outer scope and flushes at its end"_test = [] {
        ECS::CommandBuffer buffer;
        bool ran = false;
        bool ranAfterInner = true;
        bool recordingAfterInner = false;

        buffer.record([&] {
            buffer.record([&] { buffer.push([&] { ran = true; }); });
            ranAfterInner = ran;
            recordingAfterInner = buffer.recording();
        });

        expect(!ranAfterInner);
        expect(recordingAfterInner);
        expect(ran);
        expect(!buffer.recording());
    };
}

void test_commands_do_not_replay_on_next_scope() {
    "commands run once and do not replay on a later scope"_test = [] {
        ECS::CommandBuffer buffer;
        int runs = 0;

        buffer.record([&] { buffer.push([&] { ++runs; }); });
        buffer.record([] {});

        expect(runs == 1_i);
    };
}

void test_command_cannot_push_during_flush() {
    "a command pushing during the flush throws: recording has stopped"_test = [] {
        ECS::CommandBuffer buffer;

        expect(throws<std::logic_error>([&] {
            buffer.record([&] {
                buffer.push([&] { buffer.push([] {}); });
            });
        }));
        expect(buffer.size() == 0_ul);
    };
}
}

void run_command_buffer_tests() {
    test_buffer_is_not_recording_by_default();
    test_record_runs_callable_while_recording();
    test_commands_run_after_callable_in_push_order();
    test_push_queues_until_end_of_scope();
    test_push_outside_record_throws();
    test_record_without_commands_is_a_no_op();
    test_throwing_callable_still_flushes_and_rethrows();
    test_throwing_command_propagates_and_drops_the_rest();
    test_nested_record_joins_the_outer_scope();
    test_commands_do_not_replay_on_next_scope();
    test_command_cannot_push_during_flush();
}
