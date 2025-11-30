#include "raylib.h"
#include "umka_api.h"
#include <stdio.h>
#include <stdlib.h>

#define ARENA_IMPLEMENTATION
#include "span.h"
#include "raymath.h"

// TODO:
//   - [ ] Handle delay for each action

bool spc_init(const char *filename)
{
    ctx = (Context) {
        .easing = EM_Sine,
        .width = 800,
        .height = 600
    };

    char *content = NULL;
    bool ok = spu_content_w_preamble(filename, &content);
    if (!ok) {
        return false;
    }

    void *umka = umkaAlloc();
    ok = umkaInit(umka, NULL, content, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (!ok) {
        spu_print_err(umka);
        return false;
    }

    UmkaFunc fns[] = {
        (UmkaFunc){.name = "rect", .func = &spuo_rect},
        (UmkaFunc){.name = "text", .func = &spuo_text},
        (UmkaFunc){.name = "fade_in", .func = &spu_fade_in},
        (UmkaFunc){.name = "fade_out", .func = &spu_fade_out},
        (UmkaFunc){.name = "move", .func = &spu_move},
        (UmkaFunc){.name = "wait", .func = &spu_wait},
        (UmkaFunc){.name = "play", .func = &spu_play},
    };
    for (int i = 0; i < SP_LEN(fns); i++) {
        ok = umkaAddFunc(umka, fns[i].name, fns[i].func);
        if (!ok) {
            spu_print_err(umka);
            return false;
        }
    }

    ok = umkaCompile(umka);
    if (!ok) {
        spu_print_err(umka);
        return false;
    }
    spu_run_sequence(umka);
    umkaFree(umka);

    printf("ctx.tasks.count = %d\n", ctx.tasks.count);
    spc_reset_objs();

    return true;
}

void spc_update(float dt)
{
    if (ctx.current < ctx.tasks.count) {
        Task task = ctx.tasks.items[ctx.current];
        float factor = sp_easing(ctx.t, task.duration);

        if (ctx.t <= task.duration) {
            ActionList al = task.actions;
            for (int i = 0; i < al.count; i++) {
                Action a = al.items[i];

                switch (a.kind) {
                    case AK_Enable: {
                        Obj *obj = NULL;
                        SP_ASSERT(spc_get_obj(a.obj_id, &obj));
                        obj->enabled = true;
                    } break;

                    case AK_Wait: break;

                    case AK_Move: {
                        Obj *obj = {0};
                        DVector2 *pos = NULL;
                        SP_ASSERT(spc_get_obj(a.obj_id, &obj));
                        SP_ASSERT(obj->enabled);
                        spo_get_pos(obj, &pos);
                        SP_ASSERT(pos != NULL);

                        spa_interp(a, (void*)&pos, factor);
                    } break;

                    case AK_Fade: {
                        Obj *obj = {0};
                        Color *color = NULL;
                        SP_ASSERT(spc_get_obj(a.obj_id, &obj));
                        SP_ASSERT(obj->enabled);
                        spo_get_color(obj, &color);
                        SP_ASSERT(color != NULL);

                        spa_interp(a, (void*)&color, factor);
                    } break;

                    default: {
                        SP_UNREACHABLEF("Unknown kind: %d", a.kind);
                    } break;
                }
            }

            ctx.t += dt;
        } else {
            ctx.current++;
            if (ctx.current < ctx.tasks.count) {
                ctx.t = 0.0f;
                printf("current: task[%d]...\n", ctx.current);
            } else {
                printf("paused...\n");
                ctx.paused = true;
            }
        }
    }
}

int main(void)
{
    bool success = spc_init("./test.um");
    if (!success) return 1;

    InitWindow(800, 600, "span");
    SetTargetFPS(60);

    Vector2 dimension = {(f32)GetScreenWidth(), (f32)GetScreenHeight()};
    Camera2D cam = {
        .offset = Vector2Scale(dimension, 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

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
            spc_reset_objs();
        }

        if (!ctx.paused) {
            SP_ASSERT(dt_mul != 0);
            f32 mult = dt_mul > 0 ? (f32)dt_mul : 1.0 / (f32)abs(dt_mul);
            spc_update(GetFrameTime() * mult);
        }

        BeginDrawing(); {
            ClearBackground(BLACK);

            BeginMode2D(cam); {
                for (int i = 0; i < ctx.objs.count; i++) {
                    Obj *obj = NULL;
                    SP_ASSERT(spc_get_obj(i, &obj));
                    spo_render(*obj);
                }
            } EndMode2D();

            int pos[2] = {10, 10};
            DrawFPS(pos[0], pos[1]);
            DrawText(
                TextFormat(dt_mul > 0 ? "%dx" : "1/%dx", abs(dt_mul)),
                pos[0], pos[1] + 25, 20, WHITE
            );
            if (ctx.paused) DrawText("Paused", pos[0], pos[1] + 2*25, 20, WHITE);
        } EndDrawing();
    }

    CloseWindow();
    arena_free(&arena);

    return 0;
}
