#include "raylib.h"

#define ARENA_IMPLEMENTATION
#include "span.h"

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
            spc_clear_tasks();
            spc_umka_init(filename);
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

// int main2(void)
// {
//     IVector2 res = { 800, 600 };
//     SetConfigFlags(FLAG_MSAA_4X_HINT);
//     SetTraceLogLevel(LOG_WARNING);
//     InitWindow(res.x, res.y, "span - axes test");
//     SetTargetFPS(60);

//     Camera2D cam = {
//         .offset = Vector2Scale(spv_itof(res), 0.5),
//         .target = Vector2Zero(),
//         .rotation = 0.0f,
//         .zoom = 1.0f,
//     };

//     const Axes axes = spo_axes_init(Vector2Zero(), (Vector2){550.f, 550.f});
//     PointList pts = {0};
//     int n = 50;
//     f64 dx = (axes.xmax - axes.xmin) / (f32)(n-1);
//     int ind = 0;
//     Vector2 p = {0};
//     for (f64 x = axes.xmin; x <= axes.xmax; x += dx, ind++) {
//         p = (Vector2){x, 10.f*cosf(2*PI*x)-15.f};
//         p = spo_axes_plot(axes, p);
//         arena_da_append(&arena, &pts, p);
//     }
//     // Padding final value for catmull-rom spline rendering
//     arena_da_append(&arena, &pts, p);

//     while (!WindowShouldClose()) {
//         BeginDrawing();
//         ClearBackground(BLACK);

//         BeginMode2D(cam); {
//             spo_axes_render(&axes, pts, 4.5f);
//         } EndMode2D();

//         DrawFPS(10, 10);
//         EndDrawing();
//     }

//     CloseWindow();
//     return 0;
// }
