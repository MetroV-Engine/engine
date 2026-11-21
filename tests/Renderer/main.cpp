// Tests of engine_render: each file defines a run_* function, called here.
void run_offscreen_renderer_tests();
void run_bgfx_context_tests();
void run_bgfx_offscreen_tests();

int main()
{
    run_offscreen_renderer_tests();
    run_bgfx_context_tests();
    run_bgfx_offscreen_tests();
}
