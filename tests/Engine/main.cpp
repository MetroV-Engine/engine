void run_scene_tests();
void run_scene_manager_tests();
void run_scene_registry_tests();
void run_services_tests();
void run_engine_tests();
void run_data_scene_tests();
void run_uuid_tests();
void run_serialize_value_tests();
void run_load_value_tests();
void run_load_value_range_tests();
void run_locale_tests();
void run_serialize_world_tests();
void run_load_world_tests();
void run_file_tests();

int main()
{
    run_scene_tests();
    run_scene_manager_tests();
    run_scene_registry_tests();
    run_services_tests();
    run_engine_tests();
    run_data_scene_tests();
    run_uuid_tests();
    run_serialize_value_tests();
    run_load_value_tests();
    run_load_value_range_tests();
    run_locale_tests();
    run_serialize_world_tests();
    run_load_world_tests();
    run_file_tests();
}
