// Tests of engine_render: each file defines a run_* function, called here.
void run_expect_helper_tests();
void run_camera_matrix_tests();
void run_colors_tests();
void run_transform_tests();
void run_quad_builder_tests();
void run_shape_builder_tests();
void run_sprite_animation_tests();
void run_null_renderer_tests();
void run_font_tests();
void run_image_loader_tests();
void run_renderer_factory_tests();
void run_sdl_renderer_tests();
void run_offscreen_renderer_tests();
void run_bgfx_context_tests();
void run_bgfx_offscreen_tests();

int main()
{
    run_expect_helper_tests();
    run_camera_matrix_tests();
    run_colors_tests();
    run_transform_tests();
    run_quad_builder_tests();
    run_shape_builder_tests();
    run_sprite_animation_tests();
    run_null_renderer_tests();
    run_font_tests();
    run_image_loader_tests();
    run_renderer_factory_tests();
    run_sdl_renderer_tests();
    run_offscreen_renderer_tests();
    run_bgfx_context_tests();
    run_bgfx_offscreen_tests();
}
