void run_logger_send_tests();
void run_logger_filter_tests();
void run_logger_ring_buffer_tests();
void run_logger_capacity_tests();

int main()
{
    run_logger_send_tests();
    run_logger_filter_tests();
    run_logger_ring_buffer_tests();
    run_logger_capacity_tests();
}
