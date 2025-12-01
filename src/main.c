#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

#define ARENA_IMPLEMENTATION
#include "span.h"
#include "raymath.h"

// TODO:
//   - [ ] Handle delay for each action

Vector2 plot_xy(Vector2 origin, Rectangle box, f32 xmin, f32 xmax, f32 ymin, f32 ymax, Vector2 pt)
{
    SP_ASSERT(xmin <= xmax);
    // SP_ASSERT(ymin >= ymax);
    SP_ASSERT(xmin <= pt.x && pt.x <= xmax);

    return (Vector2) {
        origin.x + ((pt.x / (xmax - xmin)) * box.width),
        origin.y + ((pt.y / (ymax - ymin)) * box.height)
    };
}

int main(void)
{
    IVector2 res = { 800, 600 };
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(res.x, res.y, "span - axes test");

    Camera2D cam = {
        .offset = Vector2Scale(spv_itof(res), 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

    f32 xmin = -1.f;
    f32 xmax = 10.f;
    f32 ymin = -3.f;
    f32 ymax = 10.f;

    ymin *= -1.f;
    ymax *= -1.f;
    // f32 max = ymax;
    // ymax = ymin;
    // ymin = max;

    Vector2 sz = { 500.0f, 500.0f };
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

    Vector2 pts[400] = {0};
    int n = SP_LEN(pts);
    f32 dx = (xmax - xmin) / (f32)(n-1);
    int ind = 0;
    for (f32 x = xmin; x <= xmax; x += dx, ind++) {
        pts[ind] = (Vector2) {x, (x-3.f)*(x-3.f) + 2};
        // SP_PRINT_V2(pts[ind]);
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(cam); {
            // DrawRectangleLinesEx(box, 2.0f, RED);
            // Vertical axis
            Vector2 start = {origin_pos.x, box.y};
            Vector2 end = {origin_pos.x, box.y + box.height};
            DrawLineEx(start, end, 2.0f, RED);

            // Horizontal axis
            start = (Vector2) {box.x, origin_pos.y};
            end = (Vector2) {box.x + box.width, origin_pos.y};
            DrawLineEx(start, end, 2.0f, RED);

            for (int i = 1; i < n; i++) {
                Vector2 s = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, pts[i-1]);
                Vector2 e = plot_xy(origin_pos, box, xmin, xmax, ymin, ymax, pts[i]);

                DrawLineEx(s, e, 3.5f, BLUE);
                // DrawCircleV(s, 5.0f, WHITE);
                // DrawCircleV(e, 5.0f, WHITE);
            }
        } EndMode2D();

        EndDrawing();
    }

    CloseWindow();
}

int main3(void)
{
    IVector2 res = { 800, 600 };
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(res.x, res.y, "span - axes test");

    Camera2D cam = {
        .offset = Vector2Scale(spv_itof(res), 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

    Vector2 sz = { 500.0f, 500.0f };
    Vector2 center = {0};
    Vector2 pos = Vector2Subtract(center, Vector2Scale(sz, 0.5));
    Rectangle box = {
        .x = pos.x,
        .y = pos.y,
        .width = sz.x,
        .height = sz.y,
    };
    Vector2 origin = { box.x + 0.5*box.width, box.y + 0.5*box.height };

    f32 xmin = -10.f;
    f32 xmax = 10.f;
    f32 ymin = -10.f;
    f32 ymax = 10.f;
    f32 dx = 0.5f;
    f32 dy = 0.5f;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(cam); {
            // DrawRectangleLinesEx(box, 2.0f, RED);
            // Vertical axis
            Vector2 start = {box.x + 0.5f*box.width, box.y};
            Vector2 end = {box.x + 0.5f*box.width, box.y + box.height};
            DrawLineEx(start, end, 2.0f, RED);

            // Horizontal axis
            start = (Vector2) {box.x, box.y + 0.5f*box.height};
            end = (Vector2) {box.x + box.width, box.y + 0.5f*box.height};
            DrawLineEx(start, end, 2.0f, RED);

            for (f32 y = ymin; y <= ymax; y += dy) {
                for (f32 x = xmin; x <= xmax; x += dx) {
                    Vector2 pt = {x, y};
                    plot_xy(origin, box, xmin, xmax, ymin, ymax, pt);
                }
            }
        } EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    return 0;
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
