#include "raylib.h"
#include "umka_api.h"
#include <math.h>
#include <stdio.h>

#define ARENA_IMPLEMENTATION
#include "span.h"
#include "raymath.h"

// TODO:
//   - [ ] Improve error file line number by subtracting the # of lines in the
//   preamble
//   - [ ] Handle delay for each action

#define SCENE_OBJ ((Id)-1)

typedef struct {
    const char *name;
    UmkaExternFunc func;
} UmkaFunc;

Context ctx = {0};

Id spc_next_id(void)
{
    Id id = ctx.id_counter;
    ctx.id_counter++;
    return id;
}

void spc_new_task(f64 duration)
{
    arena_da_append(&arena, &ctx.tasks, (Task){.duration = duration});
}

void spc_add_action(Action action)
{
    if (ctx.tasks.count == 0) {
        spc_new_task(0.0);
    }

    Task *last = &ctx.tasks.items[ctx.tasks.count - 1];
    arena_da_append(&arena, &last->actions, action);
}

bool spc_get_sim_obj(Id id, Obj **obj)
{
    if (id < ctx.objs.count) {
        *obj = &ctx.sim_objs.items[id];
        return true;
    } else {
        return false;
    }
}

bool spc_get_obj(Id id, Obj **obj)
{
    if (id < ctx.objs.count) {
        *obj = &ctx.objs.items[id];
        return true;
    } else {
        return false;
    }
}

Obj spo_rect(DVector2 pos, DVector2 size, Color color)
{
    return (Obj) {
        .kind = OK_RECT,
        .enabled = false,
        .as = {
            .rect = {
                .id = spc_next_id(),
                .position = pos,
                .size = size,
                .color = color,
            }
        }
    };
}

void spo_get_pos(Obj *obj, DVector2 **pos)
{
    switch (obj->kind) {
        case OK_RECT: {
            *pos = &obj->as.rect.position;
        } break;

        default: {
            SP_UNREACHABLEF("Unknown kind of object: %d", obj->kind);
        } break;
    }
}

void spo_get_color(Obj *obj, Color **color)
{
    switch (obj->kind) {
        case OK_RECT: {
            *color = &obj->as.rect.color;
        } break;

        default: {
            SP_UNREACHABLEF("Unknown kind of object: %d", obj->kind);
        } break;
    }
}

void spo_render(Obj obj)
{
    if (!obj.enabled) return;

    switch (obj.kind) {
        case OK_RECT: {
            Rect r = obj.as.rect;
            Vector2 pos = Vector2Scale(sp_from_dv2(r.position), UNIT_TO_PX);
            Vector2 size = Vector2Scale(sp_from_dv2(r.size), UNIT_TO_PX);
            pos = Vector2Subtract(pos, Vector2Scale(size, 0.5));
            DrawRectangleV(pos, size, r.color);
        } break;

        default: {
            SP_UNREACHABLEF("Unknown kind of object: %d", obj.kind);
        } break;
    }
}

Action spo_enable(Id obj_id)
{
    return (Action) {
        .delay = 0.0,
        .obj_id = obj_id,
        .kind = AK_Enable,
        // NOTE: args should be left empty
    };
}

void spu_new_rect(UmkaStackSlot *p, UmkaStackSlot *r)
{
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 0);
    DVector2 size = *(DVector2 *)umkaGetParam(p, 1);
    Color color = *(Color *)umkaGetParam(p, 2);

    Obj rect = spo_rect(pos, size, color);
    umkaGetResult(p, r)->intVal = ctx.id_counter - 1;

    arena_da_append(&arena, &ctx.objs, rect);
    arena_da_append(&arena, &ctx.sim_objs, rect);
}

void spu_fade_in(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);

    Id obj_id = *(Id *)umkaGetParam(p, 0);
    f64 delay = *(f64 *)umkaGetParam(p, 1);

    Obj *obj = NULL;
    Color *current = NULL;
    SP_ASSERT(spc_get_sim_obj(obj_id, &obj));
    spo_get_color(obj, &current);

    FadeData fade = {
        .start = ColorAlpha(*current, 0.0),
        .end = ColorAlpha(*current, 1.0),
    };

    Action action = {
        .obj_id = obj_id,
        .delay = delay,
        .kind = AK_Fade,
        .args = {.fade = fade},
    };

    if (!obj->enabled) {
        // It need to be enabled first to be rendered on the screen.
        spc_add_action(spo_enable(obj_id));
    }
    spc_add_action(action);

    // Update the obj's prop
    *current = fade.end;
}

void spu_fade_out(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);

    Id obj_id = *(Id *)umkaGetParam(p, 0);
    f64 delay = *(f64 *)umkaGetParam(p, 1);

    Obj *obj = NULL;
    Color *current = NULL;
    SP_ASSERT(spc_get_sim_obj(obj_id, &obj));
    spo_get_color(obj, &current);

    FadeData fade = {
        .start = ColorAlpha(*current, 1.0),
        .end = ColorAlpha(*current, 0.0),
    };

    Action action = {
        .obj_id = obj_id,
        .delay = delay,
        .kind = AK_Fade,
        .args = {.fade = fade},
    };

    if (!obj->enabled) {
        // It need to be enabled first to be rendered on the screen.
        spc_add_action(spo_enable(obj_id));
    }
    spc_add_action(action);

    // Update the obj's prop
    *current = fade.end;
}

void spu_wait(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(p);
    SP_UNUSED(r);
    Action action = {
        .obj_id = SCENE_OBJ,
        .delay = 0.0,
        .kind = AK_Wait,
        // NOTE: args should be left empty
    };
    spc_add_action(action);
}

void spu_move(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);

    Id obj_id = *(Id *)umkaGetParam(p, 0);
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 1);
    f64 delay = *(f64 *)umkaGetParam(p, 2);

    Obj *obj = NULL;
    DVector2 *current = NULL;
    SP_ASSERT(spc_get_sim_obj(obj_id, &obj));
    spo_get_pos(obj, &current);

    MoveData move = {
        .start = *current,
        .end = pos,
    };

    Action action = {
        .obj_id = obj_id,
        .delay = delay,
        .kind = AK_Move,
        .args = {.move = move},
    };

    if (!obj->enabled) {
        // It need to be enabled first to be rendered on the screen.
        spc_add_action(spo_enable(obj_id));
    }
    spc_add_action(action);

    // Update the obj's prop
    *current = move.end;
}

void spu_play(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);
    f64 duration = *(f64 *)umkaGetParam(p, 0);

    Task *last = &ctx.tasks.items[ctx.tasks.count - 1];
    last->duration = duration;

    // NOTE: the line below is just a hack for now. a new task should only be added
    // when a new action is added. the line below just preemptively adds a task, which
    // is good but should not be done here.
    // TODO: remove this
    spc_new_task(0.0);
}

void spa_interp(Action action, void **value, f32 factor)
{
    if (factor < 0.0f) factor = 0.0;
    if (factor > 1.0f) factor = 1.0;

    switch (action.kind) {
        case AK_Fade: {
            Color *c = *(Color**)value;
            FadeData args = action.args.fade;
            *c = ColorLerp(args.start, args.end, factor);
        } break;

        case AK_Move: {
            DVector2 *v = *(DVector2**)value;
            MoveData args = action.args.move;
            *v = sp_to_dv2(
                Vector2Lerp(sp_from_dv2(args.start), sp_from_dv2(args.end), factor)
            );
        } break;

        default: {
            SP_UNREACHABLEF("Unknown kind of action: %d", action.kind);
        } break;
    }
}

f32 easing(f32 t, f32 duration)
{
    switch (ctx.easing) {
        case EM_Linear:
            return t / duration;

        case EM_Sine:
            return -0.5*cosf(PI / duration * t) + 0.5;

        default:
            SP_UNREACHABLEF("Unknown mode of easing: %d", ctx.easing);
    }
}

int main(void)
{
    char *content = NULL;
    bool ok = spu_content_preamble(&arena, "./test.um", &content);
    if (!ok) {
        return 1;
    }

    void *umka = umkaAlloc();
    ok = umkaInit(umka, NULL, content, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (!ok) {
        spu_print_err(umka);
        return 1;
    }

    UmkaFunc fns[] = {
        (UmkaFunc){.name = "new_rect", .func = &spu_new_rect},
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
            return 1;
        }
    }

    ok = umkaCompile(umka);
    if (!ok) {
        spu_print_err(umka);
        return 1;
    }

    spu_run_sequence(umka);

    int current = 0;
    bool paused = false;
    f32 t = 0.0f;

    InitWindow(800, 600, "span");
    SetTargetFPS(60);

    printf("ctx.tasks.count = %d\n", ctx.tasks.count);
    ctx.easing = EM_Sine;

    Vector2 dimension = {(f32)GetScreenWidth(), (f32)GetScreenHeight()};
    Camera2D cam = {
        .offset = Vector2Scale(dimension, 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };

    bool quit = false;
    while (!quit && !WindowShouldClose()) {
        if (!paused) {
            f32 dt = GetFrameTime();

            if (current < ctx.tasks.count) {
                Task task = ctx.tasks.items[current];
                float factor = easing(t, task.duration);

                if (t <= task.duration) {
                    ActionList al = task.actions;
                    for (int i = 0; i < al.count; i++) {
                        Action a = al.items[i];

                        switch (a.kind) {
                            case AK_Enable: {
                                Obj *obj = NULL;
                                SP_ASSERT(spc_get_obj(a.obj_id, &obj));
                                obj->enabled = true;
                            } break;

                            case AK_Wait: {
                            } break;

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

                    t += dt;
                } else {
                    current++;
                    if (current < ctx.tasks.count) {
                        t = 0.0f;
                        printf("current task[%d]...\n", current);
                    } else {
                        printf("paused...\n");
                        paused = true;
                        quit = true;
                    }
                }
            }
        }

        BeginDrawing(); {
            ClearBackground(BLACK);

            BeginMode2D(cam);
            for (int i = 0; i < ctx.objs.count; i++) {
                Obj *obj = NULL;
                SP_ASSERT(spc_get_obj(i, &obj));
                spo_render(*obj);
            }

            EndMode2D();

            Vector2 txt_pos = GetScreenToWorld2D((Vector2){10, 10}, cam);
            DrawFPS((int)txt_pos.x, (int)txt_pos.y);
            if (paused) DrawText("Paused", (int)txt_pos.x, (int)txt_pos.y + 25, 20, WHITE);
        } EndDrawing();
    }

    CloseWindow();
    umkaFree(umka);
    arena_free(&arena);

    return 0;
}
