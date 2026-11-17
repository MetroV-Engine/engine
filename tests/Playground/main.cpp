// Tests of the playground game (apps/playground): each file defines a run_* function, called here.
void run_player_system_tests();
void run_animation_system_tests();
void run_camera_follow_system_tests();
void run_input_mapping_tests();
void run_render_system_tests();
void run_level_tests();

int main()
{
    run_player_system_tests();
    run_animation_system_tests();
    run_camera_follow_system_tests();
    run_input_mapping_tests();
    run_render_system_tests();
    run_level_tests();
}
