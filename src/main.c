#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>

#define ARENA_IMPLEMENTATION
#include "span.h"
#include "raymath.h"

// TODO:
//   - [ ] Handle delay for each action

typedef struct {
    f64 xmin, xmax, ymin, ymax;
    Rectangle box;
    Vector2 origin_pos, coord_size, center_coord;
} Axes;

Vector2 plot_xy(Axes axes, Vector2 pt)
{
    SP_ASSERT(axes.xmin <= axes.xmax);

    return (Vector2) {
        axes.origin_pos.x + ((pt.x / (axes.xmax - axes.xmin)) * axes.box.width),
        (axes.origin_pos.y + ((pt.y / (axes.ymax - axes.ymin)) * axes.box.height)) * -1.f
    };
}

f32 foo(f32 t, f32 duration)
{
    return -0.5*cosf(PI / duration * t) + 0.5;
}

#define SPLINE_TEST 1
#define N 200
int main(void)
{
    IVector2 res = { 800, 600 };
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(res.x, res.y, "span - axes test");
    SetTargetFPS(60);

    Camera2D cam = {
        .offset = Vector2Scale(spv_itof(res), 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

    Vector2 sz = { 550.f, 550.f };
    Vector2 center = {0};
    Vector2 pos = Vector2Subtract(center, Vector2Scale(sz, 0.5));
    Axes axes = {
        .xmin = -4.f, .xmax = 4.f, .ymin = -30.f, .ymax = 5.f,
        .box = (Rectangle){
            .x = pos.x,
            .y = pos.y,
            .width = sz.x,
            .height = sz.y,
        },
    };
    axes.center_coord = (Vector2) {
        .x = 0.5f * (axes.xmax + axes.xmin),
        .y = 0.5f * (axes.ymax + axes.ymin),
    };
    axes.coord_size = (Vector2) {
        .x = axes.box.width / (int)(axes.xmax - axes.xmin),
        .y = axes.box.height / (int)(axes.ymax - axes.ymin),
    },
    axes.origin_pos = Vector2Multiply(Vector2Negate(axes.center_coord), axes.coord_size);


    PointList pts = {0};
    int n = N;
    f64 dx = (axes.xmax - axes.xmin) / (f32)(n-1);
    int ind = 0;
    Vector2 p = {0};
    for (f64 x = axes.xmin; x <= axes.xmax; x += dx, ind++) {
        p = (Vector2){x, 10.f*cosf(2*PI*x)-15.f};
#if SPLINE_TEST
        p = plot_xy(axes, p);
#endif
        arena_da_append(&arena, &pts, p);
    }
    // Padding final value for catmull-rom spline rendering
    arena_da_append(&arena, &pts, p);

    f32 thickness = 4.5f;
    Vector2 start, end;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(cam); {
            DrawRectangleLinesEx(axes.box, 2.f, LIGHTGRAY);
            if (axes.xmin <= 0.f && 0.f <= axes.xmax) {
                // Vertical axis
                start = (Vector2){axes.origin_pos.x, axes.box.y};
                end = (Vector2){axes.origin_pos.x, axes.box.y + axes.box.height};
                DrawLineEx(start, end, 2.0f, RED);
            }

            if (axes.ymin <= 0.f && 0.f <= axes.ymax) {
                // Horizontal axis
                start = (Vector2) {axes.box.x, -axes.origin_pos.y};
                end = (Vector2) {axes.box.x + axes.box.width, -axes.origin_pos.y};
                DrawLineEx(start, end, 2.0f, RED);
            }

#if SPLINE_TEST
            DrawSplineCatmullRom(pts.items, pts.count, thickness, BLUE);
#else
            for (int i = 1; i < pts.count; i++) {
                start = plot_xy(axes, pts.items[i-1]);
                end = plot_xy(axes, pts.items[i]);

                DrawLineEx(start, end, thickness, BLUE);
                DrawCircleV(start, thickness/2.f, BLUE);
            }
#endif
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
