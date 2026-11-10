void run_scalar_tests();
void run_vector_tests();
void run_kinematics_tests();
void run_dynamics_tests();
void run_gravity_tests();
void run_friction_tests();
void run_surface_tests();
void run_spring_tests();
void run_radial_tests();
void run_collision_rectangle_tests();
void run_collision_circle_tests();
void run_collision_overlap_tests();
void run_collision_collide_tests();
void run_collision_raycast_tests();
void run_collision_sweep_tests();
void run_gravity_system_tests();
void run_integration_system_tests();

int main()
{
    run_scalar_tests();
    run_vector_tests();
    run_kinematics_tests();
    run_dynamics_tests();
    run_gravity_tests();
    run_friction_tests();
    run_surface_tests();
    run_spring_tests();
    run_radial_tests();
    run_collision_rectangle_tests();
    run_collision_circle_tests();
    run_collision_overlap_tests();
    run_collision_collide_tests();
    run_collision_raycast_tests();
    run_collision_sweep_tests();
    run_gravity_system_tests();
    run_integration_system_tests();
}
