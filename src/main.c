#include "raylib.h"
#define ARENA_IMPLEMENTATION
#include "span.h"
#define NOB_IMPLEMENTATION
#include "../nob.h"

int main(void)
{
    const char *filename = "./test.um";
    bool success = spc_init(filename, RM_Preview);
    if (!success) return 1;

    while (!ctx.quit && !WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            ctx.paused = !ctx.paused;
        }
        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            ctx.dt_mul--;
            if (ctx.dt_mul == 0) ctx.dt_mul = -2;
        }
        if (IsKeyPressed(KEY_RIGHT_SHIFT)) {
            ctx.dt_mul++;
            if (ctx.dt_mul == -1) ctx.dt_mul = 1;
        }
        if (IsKeyPressed(KEY_H)) {
            spc_reset();
            printf("Restarted animation\n");
        }
        if (IsKeyPressed(KEY_C)) {
            spc_clear_for_recomp();
            spc_umka_init(filename);
            spc_run_umka();
            printf("Recompiled %s\n", filename);
        }

        if (!ctx.paused) {
            SP_ASSERT(ctx.dt_mul != 0);
            f32 mult = ctx.dt_mul > 0 ? (f32)ctx.dt_mul : 1.0 / (f32)abs(ctx.dt_mul);
            spc_update(GetFrameTime() * mult);
        }

        spc_render();
    }

    spc_deinit();

    return 0;
}
