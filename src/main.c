#include "animator.h"
#include "arena.h"
#include "ffmpeg.h"

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Animator Preview");

    anim_ctx ctx = {0};
    anim_result res = anim_ctx_init(&ctx, "./test.json");
    if (res) return res;
    SetTargetFPS(ctx.cfg.fps);

    float aspect_ratio = (float)ctx.cfg.width / (float)ctx.cfg.height;
    Vector2 sd = {GetScreenWidth(), GetScreenHeight()};
    float h = sd.y * 0.9f;
    float w = aspect_ratio * h;
    Rectangle boundary = {
        .x = w / -2.0f,
        .y = h / -2.0f,
        .width = w,
        .height = h,
    };

    Camera2D camera = {
        .target = Vector2Zero(),
        .offset = Vector2Scale(sd, 0.5f),
        .zoom = 1.0f,
        .rotation = 0.0f
    };

    printf("aspect_ratio = %.4f\n", aspect_ratio);
    printf("objs.count = %zu\n", ctx.objs.count);
    printf("actions.count = %zu\n", ctx.actions.count);
    printf("ctx.total_duration = %.2f\n", ctx.total_duration);

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            h = GetScreenHeight() * 0.9f;
            w = aspect_ratio * h;
            boundary = (Rectangle) {
                .x = 0.5f * ((float)GetScreenWidth() - w),
                .y = 0.5f * ((float)GetScreenHeight() - h),
                .width = w,
                .height = h,
            };
        }
        if (IsKeyPressed(KEY_SPACE)) {
            ctx.paused = !ctx.paused;
        } else if (IsKeyPressed(KEY_S)) {
            anim_ctx_reload(&ctx);
            printf("Reloaded script: '%s'\n", ctx.script_path);
        } else if (IsKeyPressed(KEY_R) && ctx.total_duration > 1.0) {
            ctx.duration = 0.0;
            ctx.rendering = true;
            ctx.paused = false;
            ctx.rtex = LoadRenderTexture(ctx.cfg.width, ctx.cfg.height);
            SetTraceLogLevel(LOG_WARNING);
            ctx.ffmpeg = ffmpeg_start_rendering_video(
                ctx.cfg.output_path, ctx.cfg.width, ctx.cfg.height, ctx.cfg.fps
            );
        }

        float dt = GetFrameTime();
        anim_ctx_step(&ctx, dt);

        BeginDrawing();

        if (ctx.ffmpeg) {
            BeginTextureMode(ctx.rtex);
                ClearBackground(GetColor(0x181818FF));
                for (size_t i = 0; i < ctx.objs.count; i++) {
                    anim_obj_render(&ctx.objs.items[i]);
                }
            EndTextureMode();

            ClearBackground(GetColor(0x181818FF));
            DrawText("rendering", 100, 100, 40, RED);

            Image img = LoadImageFromTexture(ctx.rtex.texture);
            if (!ffmpeg_send_frame_flipped(ctx.ffmpeg, img.data, img.width, img.height)) {
                ffmpeg_end_rendering(ctx.ffmpeg, true);
            }
            UnloadImage(img);

            if (ctx.duration >= ctx.total_duration) {
                ffmpeg_end_rendering(ctx.ffmpeg, false);
                SetTraceLogLevel(LOG_INFO);
                ctx.ffmpeg = NULL;
                ctx.rendering = false;
                break;
            } else {
                ctx.duration += dt;
            }
        } else {
            BeginMode2D(camera);
                ClearBackground(GetColor(0x181818FF));
                DrawRectangleLinesEx(boundary, 3.0, GRAY);
                for (size_t i = 0; i < ctx.objs.count; i++) {
                    anim_obj_render(&ctx.objs.items[i]);
                }
            EndMode2D();
        }


        EndDrawing();
    }

    anim_ctx_deinit(&ctx);
    CloseWindow();
    return 0;
}
