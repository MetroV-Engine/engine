void run_entity_tests();
void run_entity_manager_tests();
void run_component_id_tests();
void run_component_storage_tests();
void run_component_pool_tests();
void run_resources_tests();
void run_signature_table_tests();
void run_zipper_tests();
void run_zipper_driver_tests();
void run_view_tests();
void run_registry_tests();
void run_deferred_mutations_tests();
void run_systems_tests();

int main()
{
    run_entity_tests();
    run_entity_manager_tests();
    run_component_id_tests();
    run_component_storage_tests();
    run_component_pool_tests();
    run_resources_tests();
    run_signature_table_tests();
    run_zipper_tests();
    run_zipper_driver_tests();
    run_view_tests();
    run_deferred_mutations_tests();
    run_systems_tests();
    run_registry_tests();
}
