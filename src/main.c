#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>

#define ARENA_IMPLEMENTATION
#include "span.h"
#include "raymath.h"

// TODO:
//   - [ ] Handle delay for each action

Vector2 plot_xy(Vector2 origin, Rectangle box, f32 xmin, f32 xmax, f32 ymin, f32 ymax, DVector2 point)
{
    SP_ASSERT(xmin <= xmax);

    Vector2 pt = spv_dtof(point);
    return (Vector2) {
        origin.x + ((pt.x / (xmax - xmin)) * box.width),
        (origin.y + ((pt.y / (ymax - ymin)) * box.height)) * -1.f
        // origin.y + ((pt.y / (ymax - ymin)) * box.height)
    };
}

f32 foo(f32 t, f32 duration)
{
    return -0.5*cosf(PI / duration * t) + 0.5;
}

int main(void)
{
    IVector2 res = { 800, 600 };
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(res.x, res.y, "span - axes test");
    SetTargetFPS(60);

    // Vector2 fres = spv_itof(res);
    Camera2D cam = {
        .offset = Vector2Scale(spv_itof(res), 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

    f32 xmin = -4.f;
    f32 xmax = 4.f;
    f32 ymin = -30.f;
    f32 ymax = 5.f;

    Vector2 sz = { 550.f, 550.f };
    Vector2 center = {0};
    Vector2 pos = Vector2Subtract(center, Vector2Scale(sz, 0.5));
    Rectangle box = {
        .x = pos.x,
        .y = pos.y,
        .width = sz.x,
        .height = sz.y,
    };
    Vector2 coord_size = {
        .x = box.width / (int)(xmax - xmin),
        .y = box.height / (int)(ymax - ymin),
    };
    Vector2 center_coord = {
        .x = 0.5f * (xmax + xmin),
        .y = 0.5f * (ymax + ymin),
    };
    Vector2 origin_pos = Vector2Multiply(Vector2Negate(center_coord), coord_size);

    PointList pts = {0};
    int n = 100;
    f64 dx = (xmax - xmin) / (f32)(n-1);
    int ind = 0;
    for (f64 x = xmin; x <= xmax; x += dx, ind++) {
        DVector2 p = {x, 0.25*x*x*x*x + x*x*x/6 - (14/3)*x*x};
        arena_da_append(&arena, &pts, p);
    }

    f32 total_t = 0.f;
    f32 total_dur = 0.25f;
    f32 t = 0.f;
    f32 duration = total_dur / (f32)n;
    bool paused = true;
    Vector2 start, end;
    int prev = 0, current = 1;
    DVector2 v = pts.items[prev];
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }

        if (!paused) {
            if (t < (f32)duration) {
                t += GetFrameTime();
                v = spv_lerpd(
                    pts.items[prev], pts.items[current],
                    foo(t, duration));
            } else {
                prev = current;
                if (current < pts.count - 1) current++;
                total_t += GetFrameTime();
                t = 0.f;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(cam); {
            DrawRectangleLinesEx(box, 2.f, LIGHTGRAY);
            if (xmin <= 0.f && 0.f <= xmax) {
                // Vertical axis
                start = (Vector2){origin_pos.x, box.y};
                end = (Vector2){origin_pos.x, box.y + box.height};
                DrawLineEx(start, end, 2.0f, RED);
            }

            if (ymin <= 0.f && 0.f <= ymax) {
                // Horizontal axis
                start = (Vector2) {box.x, -origin_pos.y};
                end = (Vector2) {box.x + box.width, -origin_pos.y};
                DrawLineEx(start, end, 2.0f, RED);
            }

            for (int i = 1; i <= prev; i++) {
                Vector2 s0 = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, pts.items[i-1]);
                Vector2 e0 = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, pts.items[i]);

                DrawLineEx(s0, e0, 3.5f, BLUE);
                // DrawCircleV(s0, 5.f, WHITE);
                // DrawCircleV(e0, 5.f, WHITE);
            }

            Vector2 s1 = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, pts.items[prev]);
            Vector2 v1 = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, v);
            DrawLineEx(s1, v1, 3.5f, BLUE);
        } EndMode2D();

        DrawFPS(10, 10);
        EndDrawing();
    }

    CloseWindow();
}

int main2(void)
{
    const char *filename = "./test.um";
    bool success = spc_init(filename, RM_Output);
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
