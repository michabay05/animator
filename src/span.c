#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "span.h"
#include "raylib.h"
#include "raymath.h"

Arena arena = {0};
Context ctx = {0};

Id spc_next_id(void)
{
    Id id = ctx.id_counter;
    ctx.id_counter++;
    return id;
}

void spc_print_tasks(TaskList tl)
{
    printf("%d\n", tl.count);
    for (int i = 0; i < tl.count; i++) {
        Task t = tl.items[i];
        printf("Task {\n");
        printf("    duration = %f\n", t.duration);
        for (int k = 0; k < t.actions.count; k++) {
            Action a = t.actions.items[k];
            printf("    [%2d] {id = %d, kind = %d, delay = %f}\n", k, a.obj_id, a.kind, a.delay);
        }
        printf("}\n");
    }
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

bool spc_get_obj(Id id, Obj **obj)
{
    if (id < ctx.objs.count) {
        *obj = &ctx.objs.items[id];
        return true;
    } else {
        return false;
    }
}

void spc_reset_objs(void)
{
    for (int i = 0; i < ctx.objs.count; i++) {
        ctx.objs.items[i] = ctx.orig_objs.items[i];
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

Obj spo_text(const char *str, DVector2 pos, f32 font_size, Color color)
{
    return (Obj) {
        .kind = OK_TEXT,
        .enabled = false,
        .as = {
            .text = {
                .id = spc_next_id(),
                .str = arena_strdup(&arena, str),
                .position = pos,
                .font_size = font_size,
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

        case OK_TEXT: {
            *pos = &obj->as.text.position;
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

        case OK_TEXT: {
            *color = &obj->as.text.color;
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

        case OK_TEXT: {
            Text t = obj.as.text;
            Font font = GetFontDefault();
            f32 spacing = 2.0f;

            Vector2 pos = Vector2Scale(sp_from_dv2(t.position), UNIT_TO_PX);
            f32 font_size = t.font_size;
            Vector2 text_dim = MeasureTextEx(font, t.str, font_size, spacing);
            pos = Vector2Subtract(pos, Vector2Scale(text_dim, 0.5));
            DrawTextEx(font, t.str, pos, font_size, spacing, t.color);
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

void spu_run_sequence(void *umka)
{
    spu_call_fn(umka, "sequence", NULL, 0);
}

static void spu__preamble_count_lines(const char *preamble)
{
    int count = 1;
    for (int i = 0; i < (int)strlen(preamble); i++) {
        if (preamble[i] == '\n') count++;
    }
    ctx.preamble_lines = count;
}

void spu_print_err(void *umka)
{
    UmkaError *err = umkaGetError(umka);
    if (err->line <= ctx.preamble_lines) {
        printf("preamble:%d:%d: %s\n", err->line, err->pos, err->msg);
    } else {
        int line_no = err->line - ctx.preamble_lines;
        printf("%s:%d:%d: %s\n", err->fnName, line_no, err->pos, err->msg);
    }
}

bool spu_call_fn(void *umka, const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes)
{
    UmkaFuncContext fn = {0};
    bool umkaOk = umkaGetFunc(umka, NULL, fn_name, &fn);
    if (!umkaOk)
        return false;

    if (storage_bytes > 0) {
        umkaGetResult(fn.params, fn.result)->ptrVal = arena_alloc(&arena, storage_bytes);
    }

    umkaOk = umkaCall(umka, &fn) == 0;
    if (!umkaOk) {
        spu_print_err(umka);
        return false;
    }

    // If slot is null, then I probably don't care about the result.
    // Just the fact that the function ran
    if (slot != NULL) {
        *slot = umkaGetResult(fn.params, fn.result);
    }
    return true;
}

bool spu_content_w_preamble(const char *filename, char **content)
{
    char preamble[2*1024] = {0};

    FILE *fp = fopen("preamble.um", "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not find '%s'\n", filename);
        return false;
    }

    // Source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0L, SEEK_END);
    size_t fsz = ftell(fp);
    rewind(fp);
    fread(preamble, fsz, 1, fp);
    preamble[strlen(preamble)] = '\0';
    fclose(fp);
    spu__preamble_count_lines(preamble);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not find '%s'\n", filename);
        return false;
    }

    fseek(fp, 0L, SEEK_END);
    fsz = ftell(fp);
    rewind(fp);

    char *file_content = arena_alloc(&arena, fsz + 1);
    fread(file_content, fsz, 1, fp);
    file_content[fsz] = '\0';
    fclose(fp);

    *content = arena_sprintf(&arena, "%s\n%s", preamble, file_content);
    return true;
}

void spuo_rect(UmkaStackSlot *p, UmkaStackSlot *r)
{
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 0);
    DVector2 size = *(DVector2 *)umkaGetParam(p, 1);
    Color color = *(Color *)umkaGetParam(p, 2);

    Obj rect = spo_rect(pos, size, color);
    umkaGetResult(p, r)->intVal = ctx.id_counter - 1;

    arena_da_append(&arena, &ctx.objs, rect);
    arena_da_append(&arena, &ctx.orig_objs, rect);
}

void spuo_text(UmkaStackSlot *p, UmkaStackSlot *r)
{
    const unsigned char *text = (const unsigned char *)(umkaGetParam(p, 0)->ptrVal);
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 1);
    f32 font_size = *(f32 *)umkaGetParam(p, 2);
    Color color = *(Color *)umkaGetParam(p, 3);

    Obj rect = spo_text((const char *)text, pos, font_size, color);
    umkaGetResult(p, r)->intVal = ctx.id_counter - 1;

    arena_da_append(&arena, &ctx.objs, rect);
    arena_da_append(&arena, &ctx.orig_objs, rect);
}

void spu_fade_in(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);

    Id obj_id = *(Id *)umkaGetParam(p, 0);
    f64 delay = *(f64 *)umkaGetParam(p, 1);

    Obj *obj = NULL;
    Color *current = NULL;
    SP_ASSERT(spc_get_obj(obj_id, &obj));
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
    SP_ASSERT(spc_get_obj(obj_id, &obj));
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
    SP_ASSERT(spc_get_obj(obj_id, &obj));
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
    factor = Clamp(factor, 0.0f, 1.0f);

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

f32 sp_easing(f32 t, f32 duration)
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

Vector2 sp_from_dv2(DVector2 dv)
{
    return (Vector2){
        .x = (f32)dv.x,
        .y = (f32)dv.y,
    };
}

DVector2 sp_to_dv2(Vector2 v)
{
    return (DVector2){
        .x = (f64)v.x,
        .y = (f64)v.y,
    };
}
