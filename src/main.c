#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"
#include "umka_api.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

// TODO:
//   - [ ] Improve error file line number by subtracting the # of lines in the
//   preamble
//   - [ ] Handle delay for each action

#define UNREACHABLE(message, ...)                                                                  \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: UNREACHABLE: " message "\n", __FILE__, __LINE__, __VA_ARGS__);     \
        abort();                                                                                   \
    } while (0)

#define print_v2(v) (printf("%s = (%f, %f)\n", #v, v.x, v.y))
#define print_clr(c) (printf("%s = (%d, %d, %d, %d)\n", #c, c.r, c.g, c.b, c.a))

typedef float f32;
typedef double f64;
typedef uint16_t Id;

typedef struct {
    f64 x, y;
} DVector2;

typedef struct {
    Id id;
    Vector2 position;
    Vector2 size;
    Color color;
} Rect;

// NOTE: Don't rearrange order without modifying Umka enum
typedef enum {
    AK_Sleep,
    AK_FadeIn,
    AK_FadeOut,
} ActionKind;

typedef struct {
    int kind;
    f64 delay;
} Action;

typedef struct {
    Action *items;
    int count;
    int capacity;
} ActionList;

typedef struct {
    ActionList actions;
    f64 duration;
} Task;

typedef struct {
    Task *items;
    int count;
    int capacity;
} TaskList;

typedef struct {
    UmkaDynArray(Action) actions;
    f64 duration;
} UmkaTask;

Vector2 from_dv2(DVector2 dv)
{
    return (Vector2){
        .x = (f32)dv.x,
        .y = (f32)dv.y,
    };
}

Arena arena = {0};

void print_err(void *umka)
{
    UmkaError *err = umkaGetError(umka);
    printf("%s:%d:%d: %s\n", err->fnName, err->line, err->pos, err->msg);
}

bool call_umka_fn(void *umka, const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes)
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
        print_err(umka);
        return false;
    }

    // If slot is null, then I probably don't care about the result.
    // Just the fact that the function ran
    if (slot != NULL) {
        *slot = umkaGetResult(fn.params, fn.result);
    }
    return true;
}

bool get_tasks(void *umka, TaskList *tasks)
{
    UmkaStackSlot *slot = NULL;
    call_umka_fn(umka, "sequence", NULL, 0);
    printf("Sequence done...\n");

    slot = NULL;
    call_umka_fn(umka, "get_task_count", &slot, 0);
    int n = (int)slot->intVal;
    printf("n = %d\n", n);

    slot = NULL;
    call_umka_fn(umka, "get_tasks", &slot, n * sizeof(UmkaTask));

    UmkaTask *dt = (UmkaTask *)slot->ptrVal;
    TaskList tl = {0};

    for (int j = 0; j < n; j++) {
        UmkaTask ut = dt[j];
        Task t = {0};
        t.duration = ut.duration;

        for (int i = 0; i < umkaGetDynArrayLen(&ut.actions); i++) {
            Action a = {
                .kind = ut.actions.data[i].kind,
                .delay = ut.actions.data[i].delay,
            };
            arena_da_append(&arena, &t.actions, a);
        }

        arena_da_append(&arena, &tl, t);
    }

    *tasks = tl;
    return true;
}

void print_tasks(TaskList tl)
{
    printf("%d\n", tl.count);
    for (int i = 0; i < tl.count; i++) {
        Task t = tl.items[i];
        printf("Task {\n");
        printf("    duration = %f\n", t.duration);
        for (int k = 0; k < t.actions.count; k++) {
            Action a = t.actions.items[k];
            printf("    [%2d] {kind = %d, delay = %f}\n", k, a.kind, a.delay);
        }
        printf("}\n");
    }
}

static Id _id_ = 0;
static Rect rect = {0};
void __newRect(UmkaStackSlot *p, UmkaStackSlot *r)
{
    DVector2 pos = *(DVector2 *)umkaGetParam(p, 0);
    DVector2 size = *(DVector2 *)umkaGetParam(p, 1);
    Color color = *(Color *)umkaGetParam(p, 2);

    rect = (Rect){
        .id = _id_,
        .position = from_dv2(pos),
        .size = from_dv2(size),
        .color = color,
    };
    _id_++;
    *(Rect *)umkaGetResult(p, r)->ptrVal = rect;
}

bool content_w_preamble(Arena *arena, const char *filename, char **content)
{
    char preamble[1024] = {0};
    const Id MAX_TASK_COUNT = 5;

    snprintf(preamble, 1024,
             "type Vec2 = struct { x, y: real };\n"
             "type Color = struct { r, g, b, a: uint8 };\n"
             "type Id = uint16;\n"
             "type Rect = struct { id: Id; pos, size: Vec2; color: Color };\n"
             "type ActionKind = enum (int32) { Sleep; FadeIn; FadeOut; };\n"
             "type Action = struct { kind: ActionKind; delay: real };\n"
             "type Task = struct { actions: []Action; duration: real }\n\n"

             "const MAX_TASK_COUNT = Id(%d);\n"
             "task_count := uint16(0);\n"
             "var ALL_TASKS: [MAX_TASK_COUNT]Task;\n\n"

             "fn newRect(\n"
             "    pos: Vec2 = Vec2{0, 0}, size: Vec2 = Vec2{1, 1},\n"
             "    color: Color = Color{255, 255, 255, 255}"
             "): Rect\n",
             MAX_TASK_COUNT);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not find '%s'\n", filename);
        return false;
    }

    // Source - https://stackoverflow.com/a
    // Posted by Rob Walker, modified by community. See post 'Timeline' for change
    // history Retrieved 2025-11-29, License - CC BY-SA 3.0
    fseek(fp, 0L, SEEK_END);
    size_t fsz = ftell(fp);
    rewind(fp);

    char *file_content = arena_alloc(arena, fsz + 1);
    fread(file_content, fsz, 1, fp);

    *content = arena_sprintf(arena, "%s\n%s", preamble, file_content);
    return true;
}

int main(void)
{
    char *content = NULL;
    bool ok = content_w_preamble(&arena, "./test.um", &content);
    if (!ok) {
        return 1;
    }

    void *umka = umkaAlloc();
    ok = umkaInit(umka, NULL, content, 1024 * 1024, NULL, 0, NULL, false, false, NULL);
    if (!ok) {
        print_err(umka);
        return 1;
    }

    ok = umkaAddFunc(umka, "newRect", &__newRect);
    if (!ok) {
        print_err(umka);
        return 1;
    }

    ok = umkaCompile(umka);
    if (!ok) {
        print_err(umka);
        return 1;
    }

    TaskList tl = {0};
    get_tasks(umka, &tl);
    printf("[INFO] Found %d tasks\n", tl.count);
    print_tasks(tl);
    int current = 0;
    bool paused = false;
    bool first = true;
    f32 t = 0.0f;

    InitWindow(800, 600, "pas");
    SetTargetFPS(60);

    const int UNIT_TO_PX = 50;
    Vector2 dimension = {(f32)GetScreenWidth(), (f32)GetScreenHeight()};
    Camera2D cam = {
        .offset = Vector2Scale(dimension, 0.5),
        .target = Vector2Zero(),
        .rotation = 0.0f,
        .zoom = 1.0f,
    };
    Color start = rect.color;

    while (!WindowShouldClose()) {
        if (!paused) {
            f32 dt = GetFrameTime();

            if (current < tl.count) {
                Task task = tl.items[current];
                if (first) {
                    start = ColorAlpha(start, 1.0f);
                    first = false;
                }

                if (t <= task.duration + dt) {
                    ActionList al = task.actions;
                    for (int i = 0; i < al.count; i++) {
                        Action a = al.items[i];
                        switch (a.kind) {
                            case AK_Sleep:
                                printf("sleep: t = %f\n", t);
                                break;

                            case AK_FadeIn:
                                rect.color = ColorLerp(
                                    ColorAlpha(start, 0.0f),
                                    ColorAlpha(start, 1.0f),
                                    t / task.duration
                                );
                                break;

                            case AK_FadeOut:
                                rect.color = ColorLerp(
                                    ColorAlpha(start, 1.0f),
                                    ColorAlpha(start, 0.0f),
                                    t / task.duration
                                );
                                break;

                            default:
                                UNREACHABLE("Unknown kind: %d", a.kind);
                                break;
                        }
                    }

                    t += dt;
                } else {
                    if (!first) first = true;
                    if (current < tl.count) {
                        current++;
                        t = 0.0f;
                        printf("current task[%d]...\n", current);
                    } else {
                        printf("paused...\n");
                        paused = true;
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(cam);
        Vector2 pos = Vector2Scale(rect.position, UNIT_TO_PX);
        Vector2 size = Vector2Scale(rect.size, UNIT_TO_PX);
        pos = Vector2Subtract(pos, Vector2Scale(size, 0.5));
        DrawRectangleV(pos, size, rect.color);

        Vector2 txt_pos = GetScreenToWorld2D((Vector2){10, 10}, cam);
        DrawFPS((int)txt_pos.x, (int)txt_pos.y);
        if (paused) DrawText("Paused", (int)txt_pos.x, (int)txt_pos.y + 25, 20, WHITE);
        EndMode2D();

        EndDrawing();
    }

    CloseWindow();
    umkaFree(umka);
    arena_free(&arena);
}
