#include "raylib.h"
#include "umka_api.h"
#include <stdio.h>
#include <stdlib.h>

#define ARENA_IMPLEMENTATION
#include "span.h"

// TODO:
//   - [ ] Handle delay for each action

int main(void)
{
    const char *filename = "./test.um";
    bool success = spc_init(filename);
    if (!success) return 1;

    int dt_mul = 1;
    while (!ctx.quit && !WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            ctx.paused = !ctx.paused;
        }
        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            dt_mul--;
            if (dt_mul == 0) dt_mul = -2;
        }
        if (IsKeyPressed(KEY_RIGHT_SHIFT)) {
            dt_mul++;
            if (dt_mul == -1) dt_mul = 1;
        }
        if (IsKeyPressed(KEY_R)) {
            spc_reset();
            printf("Restarted animation\n");
        }
        if (IsKeyPressed(KEY_C)) {
            spc_clear_tasks();
            spc_umka_init(filename);
            printf("Recompiled %s\n", filename);
        }

        if (!ctx.paused) {
            SP_ASSERT(dt_mul != 0);
            f32 mult = dt_mul > 0 ? (f32)dt_mul : 1.0 / (f32)abs(dt_mul);
            spc_update(GetFrameTime() * mult);
        }

        BeginDrawing(); {
            spc_render();

            int pos[2] = {10, 10};
            DrawFPS(pos[0], pos[1]);
            DrawText(
                TextFormat(dt_mul > 0 ? "%dx" : "1/%dx", abs(dt_mul)),
                pos[0], pos[1] + 25, 20, WHITE
            );
            if (ctx.paused) DrawText("Paused", pos[0], pos[1] + 2*25, 20, WHITE);
        } EndDrawing();
    }

    spc_deinit();

    return 0;
}
