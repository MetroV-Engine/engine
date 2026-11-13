void run_logger_send_tests();
void run_logger_filter_tests();
void run_logger_ring_buffer_tests();
void run_logger_capacity_tests();
void run_logger_output_tests();
void run_logger_concurrency_tests();
void run_logger_write_failure_tests();
void run_logger_out_of_memory_tests();

int main()
{
    run_logger_send_tests();
    run_logger_filter_tests();
    run_logger_ring_buffer_tests();
    run_logger_capacity_tests();
    run_logger_output_tests();
    run_logger_concurrency_tests();
    run_logger_write_failure_tests();
    run_logger_out_of_memory_tests();
}
