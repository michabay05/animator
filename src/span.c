#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "span.h"
#include "ffmpeg.h"
#include "raylib.h"
#include "raymath.h"
#include "umka_api.h"
#include "../nob.h"

Arena arena = {0};
Context ctx = {0};

bool spc_init(const char *filename, RenderMode mode)
{
    // NOTE: The initialization goes through three steps:
    //   (1) Umka: compile script, load objs and actions
    //   (2) Raylib: initialize window and other related things

    ctx = (Context){0};
    ctx.umka = NULL;
    ctx.easing = EM_Sine;
    ctx.dt_mul = 1;

    bool ok = spc_umka_init(filename);
    if (!ok) return false;

    spc_renderer_init(mode);

    spc_run_umka();
    return true;
}

bool spc_umka_init(const char *filename)
{
    char *content = NULL;
    bool ok = spu_content_w_preamble(filename, &content);
    if (!ok) {
        return false;
    }

    if (ctx.umka == NULL) {
        ctx.umka = umkaAlloc();
    }
    ok = umkaInit(ctx.umka, NULL, content, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (!ok) {
        spu_print_err();
        return false;
    }

    UmkaFunc fns[] = {
        (UmkaFunc){.name = "rect", .func = &spuo_rect},
        (UmkaFunc){.name = "text", .func = &spuo_text},
        (UmkaFunc){.name = "axes", .func = &spuo_axes},
        (UmkaFunc){.name = "curve", .func = &spuo_curve},
        (UmkaFunc){.name = "typst", .func = &spuo_typst},

        (UmkaFunc){.name = "fade_in", .func = &spu_fade_in},
        (UmkaFunc){.name = "fade_out", .func = &spu_fade_out},
        (UmkaFunc){.name = "move", .func = &spu_move},
        (UmkaFunc){.name = "wait", .func = &spu_wait},
        (UmkaFunc){.name = "play", .func = &spu_play},

        (UmkaFunc){.name = "enable", .func = &spuo_enable},
    };
    for (int i = 0; i < SP_LEN(fns); i++) {
        ok = umkaAddFunc(ctx.umka, fns[i].name, fns[i].func);
        if (!ok) {
            spu_print_err();
            return false;
        }
    }

    ok = umkaCompile(ctx.umka);
    if (!ok) {
        spu_print_err();
        return false;
    }
    return true;
}

void spc_renderer_init(RenderMode mode)
{
    ctx.pres = (IVector2){ 800, 600 };
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(ctx.pres.x, ctx.pres.y, "span");
    ctx.fps = 60;
    SetTargetFPS(ctx.fps);

    switch (mode) {
        case RM_Preview: {
            ctx.vres = ctx.pres;
        } break;

        case RM_Output: {
            ctx.vres = (IVector2){ 1600, 1200 };
            // NOTE: The aspect ratio of the preview window and video have to be the same.
            f32 p_aspect_ratio = (f32)ctx.pres.x / (f32)ctx.pres.y;
            f32 v_aspect_ratio = (f32)ctx.vres.x / (f32)ctx.vres.y;
            SP_ASSERT(p_aspect_ratio == v_aspect_ratio);

            ctx.rtex = LoadRenderTexture(ctx.vres.x, ctx.vres.y);
            ctx.ffmpeg = ffmpeg_start_rendering_video(
                "out.mov", (size_t)ctx.vres.x, (size_t)ctx.vres.y, (size_t)ctx.fps);
        } break;

        default: {
            SP_UNREACHABLEF("Unknown mode of render: %d", mode);
        } break;
    }
    ctx.render_mode = mode;

    ctx.cam = (Camera2D) {
        .offset = Vector2Scale(spv_itof(ctx.vres), 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };
}

void spc_run_umka(void)
{
    spu_run_sequence();
    spc_reset();
}


void spc_deinit(void)
{
    umkaFree(ctx.umka);

    if (ctx.render_mode == RM_Output) {
        ffmpeg_end_rendering(ctx.ffmpeg, false);
        UnloadRenderTexture(ctx.rtex);
    }
    CloseWindow();

    arena_free(&arena);
}

void spc_update(f32 dt)
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
            } else {
                ctx.paused = true;
            }
        }
    }
}

static void spc__main_render(void)
{
    ClearBackground(BLACK);

    BeginMode2D(ctx.cam); {
        for (int i = 0; i < ctx.objs.count; i++) {
            Obj *obj = NULL;
            SP_ASSERT(spc_get_obj(i, &obj));
            spo_render(*obj);
        }
    } EndMode2D();
}

static void spc__preview_render(void)
{
    BeginDrawing(); {
        spc__main_render();

        IVector2 pos = {10, 10};
        DrawFPS(pos.x, pos.y);
        DrawText(
            TextFormat(ctx.dt_mul > 0 ? "%dx" : "1/%dx", abs(ctx.dt_mul)),
            pos.x, pos.y + 25, 20, WHITE
        );
        if (ctx.paused) DrawText("Paused", pos.x, pos.y + 2*25, 20, WHITE);
    } EndDrawing();
}

static void spc__output_render(void)
{
    // Render to the render texture
    BeginTextureMode(ctx.rtex); {
        spc__main_render();

        SetTraceLogLevel(LOG_WARNING);
        Image image = LoadImageFromTexture(ctx.rtex.texture);
        SetTraceLogLevel(LOG_INFO);
        if (!ffmpeg_send_frame(ctx.ffmpeg, image.data, image.width, image.height)) {
            ffmpeg_end_rendering(ctx.ffmpeg, true);
        }
        UnloadImage(image);
    } EndTextureMode();

    // Render to preview window
    BeginDrawing(); {
        f32 font_size = 40;
        f32 spacing = 2.0;
        Font font = GetFontDefault();

        const char *text = "Rendering...";
        Vector2 text_dim = MeasureTextEx(font, text, font_size, spacing);
        Vector2 pos = Vector2Scale(spv_itof(ctx.pres), 0.5);
        pos = Vector2Subtract(pos, Vector2Scale(text_dim, 0.5));

        DrawTextEx(font, text, pos, font_size, spacing, WHITE);
    } EndDrawing();
}

void spc_render(void)
{
    switch (ctx.render_mode) {
        case RM_Preview: {
            spc__preview_render();
        } break;

        case RM_Output: {
            spc__output_render();
        } break;

        default: {
            SP_UNREACHABLEF("Unknown mode of render: %d", ctx.render_mode);
        } break;
    }
}

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
        // NOTE: Added a task here to make sure the code below will have at
        // least one task to attach the action to.
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

void spc_reset(void)
{
    for (int i = 0; i < ctx.objs.count; i++) {
        ctx.objs.items[i] = ctx.orig_objs.items[i];
    }
    ctx.current = 0;
    ctx.t = 0.0f;
    ctx.paused = false;
    ctx.quit = false;
}

// NOTE: For tasks, clear does not mean clear the list and free the memory.
void spc_clear_for_recomp(void)
{
    // NOTE (cont.): It just means that I will move the count variable
    // to zero so as to write new tasks over the old ones. The memory
    // will cleaned up at the end.
    ctx.tasks.count = 0;

    for (int i = 0; i < ctx.objs.count; i++) {
        Obj *o = &ctx.objs.items[i];
        switch (o->kind) {
            case OK_TYPST: {
                UnloadTexture(o->as.typst.texture);
            }

            default: break;
        }
    }
}

Obj spo_rect(DVector2 pos, DVector2 size, Color color)
{
    return (Obj) {
        .id = spc_next_id(),
        .kind = OK_RECT,
        .enabled = false,
        .as = {
            .rect = {
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
        .id = spc_next_id(),
        .kind = OK_TEXT,
        .enabled = false,
        .as = {
            .text = {
                .str = arena_strdup(&arena, str),
                .position = pos,
                .font_size = font_size,
                .color = color,
            }
        }
    };
}

Obj spo_axes(Vector2 center, f32 xmin, f32 xmax, f32 ymin, f32 ymax)
{
    Vector2 size = {550.f, 550.f};
    Vector2 pos = Vector2Subtract(center, Vector2Scale(size, 0.5));

    Axes axes = {
        .xmin = xmin, .xmax = xmax, .ymin = ymin, .ymax = ymax,
        .box = (Rectangle){
            .x = pos.x,
            .y = pos.y,
            .width = size.x,
            .height = size.y,
        },
    };
    axes.center_coord = (Vector2) {
        .x = 0.5f * (axes.xmax + axes.xmin),
        .y = 0.5f * (axes.ymax + axes.ymin),
    };
    axes.coord_size = (Vector2) {
        .x = axes.box.width / (int)(axes.xmax - axes.xmin),
        .y = axes.box.height / (int)(axes.ymax - axes.ymin),
    };
    axes.origin_pos = Vector2Multiply(
        Vector2Subtract(center, axes.center_coord),
        axes.coord_size
    );

    return (Obj){
        .id = spc_next_id(),
        .kind = OK_AXES,
        .enabled = false,
        .as = { .axes = axes },
    };
}

Obj spo_typst(const char *text, f32 font_size, DVector2 pos, Color color)
{
    Typst typ = {
        .text = text,
        .font_size = font_size,
        .position = pos,
        .color = color,
    };
    spo_typst_compile(&typ);

    return (Obj){
        .id = spc_next_id(),
        .kind = OK_TYPST,
        .enabled = false,
        .as = { .typst = typ },
    };
}

bool spo_typst_compile(Typst *typ)
{
    if (!IsWindowReady()) {
        printf("Chillout bruv...\n");
        return false;
    }

    Nob_String_Builder sb = {0};
    nob_sb_appendf(&sb,
        "#set page(width: auto, height: auto, margin: 0in, fill: none)\n"
        "#set text(size: %fpt, fill: white)\n"
        "$ %s $\n", typ->font_size , typ->text);

    bool ok = nob_write_entire_file("input.typ", sb.items, sb.count);
    if (!ok) return false;
    nob_sb_free(sb);

    Nob_Cmd cmd = {0};
    const char *output_path = "output.png";
    nob_cmd_append(&cmd, "typst", "c", "input.typ", output_path);
    if (!nob_cmd_run_sync(cmd)) {
        printf("Failed to run command\n");
        return false;
    }

    typ->texture = LoadTexture(output_path);
    SetTextureFilter(typ->texture, TEXTURE_FILTER_BILINEAR);
    return true;
}

static Vector2 spo_curve_plot(const Axes *const axes, Vector2 pt)
{
    SP_ASSERT(axes->xmin <= axes->xmax);

    return (Vector2) {
        axes->origin_pos.x + ((pt.x / (axes->xmax - axes->xmin)) * axes->box.width),
        (axes->origin_pos.y + ((pt.y / (axes->ymax - axes->ymin)) * axes->box.height)) * -1.f
    };
}

Obj spo_curve(Id axes_id)
{
    Obj axes_obj = ctx.objs.items[axes_id];
    SP_ASSERT(axes_obj.kind == OK_AXES);
    const Axes *axes = &axes_obj.as.axes;

    PointList pts = {0};
    int n = 350;
    f64 dx = (axes->xmax - axes->xmin) / (f32)n;
    Vector2 p = {0};
    for (f64 x = axes->xmin; x <= axes->xmax; x += dx) {
        p = (Vector2){x, x*x - 1.f};
        p = spo_curve_plot(axes, p);
        arena_da_append(&arena, &pts, p);
    }
    // NOTE: Padding final value for catmull-rom spline rendering is required
    // to ensure that the final point gets rendered
    arena_da_append(&arena, &pts, p);

    return (Obj) {
        .id = spc_next_id(),
        .enabled = false,
        .kind = OK_CURVE,
        .as = {
            .curve = {
                .axes_id = axes_id,
                .pts = pts,
                .color = BLUE,
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

        case OK_TYPST: {
            *pos = &obj->as.typst.position;
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

        case OK_TYPST: {
            *color = &obj->as.typst.color;
        } break;

        default: {
            SP_UNREACHABLEF("Unknown kind of object: %d", obj->kind);
        } break;
    }
}

// NOTE: This is only used for the font value for now
static f32 spv__adjusted_value(f32 f)
{
    return (f / (f32)ctx.pres.y) * ctx.vres.y;
}

static Vector2 spv__adjusted_coords(Vector2 v)
{
    return Vector2Multiply(
        Vector2Divide(v, spv_itof(ctx.pres)),
        spv_itof(ctx.vres)
    );
}

void spo_render(Obj obj)
{
    if (!obj.enabled) return;

    switch (obj.kind) {
        case OK_RECT: {
            Rect r = obj.as.rect;
            Vector2 pos = Vector2Scale(spv_dtof(r.position), UNIT_TO_PX);
            Vector2 size = Vector2Scale(spv_dtof(r.size), UNIT_TO_PX);
            pos = Vector2Subtract(pos, Vector2Scale(size, 0.5));

            DrawRectangleV(
                spv__adjusted_coords(pos),
                spv__adjusted_coords(size),
                r.color);
        } break;

        case OK_TEXT: {
            Text t = obj.as.text;
            Font font = GetFontDefault();
            f32 spacing = 2.0f;

            Vector2 pos = Vector2Scale(spv_dtof(t.position), UNIT_TO_PX);
            f32 font_size = t.font_size;
            Vector2 text_dim = MeasureTextEx(font, t.str, font_size, spacing);
            pos = Vector2Subtract(pos, Vector2Scale(text_dim, 0.5));

            DrawTextEx(
                font, t.str,
                spv__adjusted_coords(pos),
                spv__adjusted_value(font_size),
                spv__adjusted_value(spacing),
                t.color);
        } break;

        case OK_AXES: {
            const Axes *axes = &obj.as.axes;
            Vector2 start, end;
            f32 thickness = 2.f;
            // NOTE: Boundary rectangle - render only for debug purposes
            // DrawRectangleLinesEx(axes->box, 2.f, LIGHTGRAY);

            if (axes->xmin <= 0.f && 0.f <= axes->xmax) {
                // Vertical axis
                start = (Vector2){axes->origin_pos.x, axes->box.y};
                end = (Vector2){axes->origin_pos.x, axes->box.y + axes->box.height};
                DrawLineEx(start, end, thickness, RED);
            }

            if (axes->ymin <= 0.f && 0.f <= axes->ymax) {
                // Horizontal axis
                start = (Vector2) {axes->box.x, -axes->origin_pos.y};
                end = (Vector2) {axes->box.x + axes->box.width, -axes->origin_pos.y};
                DrawLineEx(start, end, thickness, RED);
            }
        } break;

        case OK_CURVE: {
            Curve c = obj.as.curve;
            DrawSplineCatmullRom(c.pts.items, c.pts.count, 4.f, c.color);
        } break;

        case OK_TYPST: {
            Typst t = obj.as.typst;
            IVector2 tex_dim = {t.texture.width, t.texture.height};
            Vector2 pos = Vector2Subtract(
                Vector2Scale(spv_dtof(t.position), UNIT_TO_PX),
                Vector2Scale(spv_itof(tex_dim), 0.5));
            DrawTextureV(t.texture, pos, t.color);
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

void spu_run_sequence(void)
{
    spu_call_fn("sequence", NULL, 0);
}

static void spu__preamble_count_lines(const char *preamble)
{
    int count = 1;
    for (int i = 0; i < (int)strlen(preamble); i++) {
        if (preamble[i] == '\n') count++;
    }
    ctx.preamble_lines = count;
}

void spu_print_err(void)
{
    UmkaError *err = umkaGetError(ctx.umka);
    if (err->line <= ctx.preamble_lines) {
        printf("preamble:%d:%d: %s\n", err->line, err->pos, err->msg);
    } else {
        int line_no = err->line - ctx.preamble_lines;
        printf("%s:%d:%d: %s\n", err->fnName, line_no, err->pos, err->msg);
    }
}

bool spu_call_fn(const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes)
{
    UmkaFuncContext fn = {0};
    bool umkaOk = umkaGetFunc(ctx.umka, NULL, fn_name, &fn);
    if (!umkaOk)
        return false;

    if (storage_bytes > 0) {
        umkaGetResult(fn.params, fn.result)->ptrVal = arena_alloc(&arena, storage_bytes);
    }

    umkaOk = umkaCall(ctx.umka, &fn) == 0;
    if (!umkaOk) {
        spu_print_err();
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
    umkaGetResult(p, r)->intVal = rect.id;

    arena_da_append(&arena, &ctx.objs, rect);
    arena_da_append(&arena, &ctx.orig_objs, rect);
}

void spuo_text(UmkaStackSlot *p, UmkaStackSlot *r)
{
    const unsigned char *text_str = (const unsigned char *)(umkaGetParam(p, 0)->ptrVal);
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 1);
    f32 font_size = *(f32 *)umkaGetParam(p, 2);
    Color color = *(Color *)umkaGetParam(p, 3);

    Obj text = spo_text((const char *)text_str, pos, font_size, color);
    umkaGetResult(p, r)->intVal = text.id;

    arena_da_append(&arena, &ctx.objs, text);
    arena_da_append(&arena, &ctx.orig_objs, text);
}

void spuo_axes(UmkaStackSlot *p, UmkaStackSlot *r)
{
    DVector2 center = *(DVector2 *)umkaGetParam(p, 0);
    f64 xmin = *(f64 *)umkaGetParam(p, 1);
    f64 xmax = *(f64 *)umkaGetParam(p, 2);
    f64 ymin = *(f64 *)umkaGetParam(p, 3);
    f64 ymax = *(f64 *)umkaGetParam(p, 4);

    Obj axes = spo_axes(spv_dtof(center), xmin, xmax, ymin, ymax);
    umkaGetResult(p, r)->intVal = axes.id;

    arena_da_append(&arena, &ctx.objs, axes);
    arena_da_append(&arena, &ctx.orig_objs, axes);
}

void spuo_curve(UmkaStackSlot *p, UmkaStackSlot *r)
{
    Id axes_id = *(Id *)umkaGetParam(p, 0);

    Obj curve = spo_curve(axes_id);
    umkaGetResult(p, r)->intVal = curve.id;

    arena_da_append(&arena, &ctx.objs, curve);
    arena_da_append(&arena, &ctx.orig_objs, curve);
}

void spuo_typst(UmkaStackSlot *p, UmkaStackSlot *r)
{
    const unsigned char *text = (const unsigned char *)(umkaGetParam(p, 0)->ptrVal);
    f32 font_size = (f32)umkaGetParam(p, 1)->realVal;
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 2);
    Color color = *(Color *)umkaGetParam(p, 3);

    Obj typst = spo_typst((const char *)text, font_size, pos, color);
    umkaGetResult(p, r)->intVal = typst.id;

    arena_da_append(&arena, &ctx.objs, typst);
    arena_da_append(&arena, &ctx.orig_objs, typst);
}

void spuo_enable(UmkaStackSlot *p, UmkaStackSlot *r)
{
    SP_UNUSED(r);
    Id obj_id = *(Id *)umkaGetParam(p, 0);

    spc_add_action(spo_enable(obj_id));
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
            *v = spv_ftod(
                Vector2Lerp(spv_dtof(args.start), spv_dtof(args.end), factor)
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

Vector2 spv_dtof(DVector2 dv)
{
    return (Vector2){
        .x = (f32)dv.x,
        .y = (f32)dv.y,
    };
}

DVector2 spv_ftod(Vector2 v)
{
    return (DVector2){
        .x = (f64)v.x,
        .y = (f64)v.y,
    };
}

Vector2 spv_itof(IVector2 iv)
{
    return (Vector2){
        .x = (f32)iv.x,
        .y = (f32)iv.y,
    };
}

DVector2 spv_lerpd(DVector2 start, DVector2 end, f64 factor)
{
    if (factor < 0.0) factor = 0.0;
    if (factor > 1.0) factor = 1.0;

    return (DVector2) {
        .x = start.x + (end.x - start.x)*factor,
        .y = start.y + (end.y - start.y)*factor,
    };
}
