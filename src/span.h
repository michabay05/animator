#ifndef _SPAN_H_
#define _SPAN_H_

#include <stdint.h>
#include "raylib.h"
#include "arena.h"
#include "umka_api.h"

#define SP_LEN(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define SP_ASSERT assert
#define SP_UNUSED(x) ((void)x)
#define SP_UNREACHABLE(message)                                                                    \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);     \
        abort();                                                                                   \
    } while (0)
#define SP_UNREACHABLEF(message, ...)                                                              \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: UNREACHABLE: " message "\n", __FILE__, __LINE__, __VA_ARGS__);     \
        abort();                                                                                   \
    } while (0)

#define SP_PRINT_V2(v) (printf("%s = (%f, %f)\n", #v, v.x, v.y))
#define SP_PRINT_CLR(c) (printf("%s = (%d, %d, %d, %d)\n", #c, c.r, c.g, c.b, c.a))
#define SP_STRUCT_ARR(st_name, type) \
    typedef struct {   \
        type *items;   \
        int count;     \
        int capacity;  \
    } st_name


typedef float f32;
typedef double f64;
typedef uint16_t Id;

typedef struct {
    f64 x, y;
} DVector2;

// NOTE: Don't rearrange order without modifying Umka enum
typedef enum {
    AK_Enable,
    AK_Wait,
    AK_Fade,
} ActionKind;

typedef struct {
    Color start;
    Color end;
} FadeData;

typedef struct {
    Id obj_id;
    ActionKind kind;
    f64 delay;
    union {
        FadeData fade;
    } args;
} Action;
SP_STRUCT_ARR(ActionList, Action);

typedef struct {
    UmkaDynArray(Action) actions;
    f64 duration;
} UmkaTask;

typedef struct {
    ActionList actions;
    f64 duration;
} Task;
SP_STRUCT_ARR(TaskList, Task);

typedef struct {
    Id id;
    DVector2 position;
    DVector2 size;
    Color color;
} Rect;

typedef enum {
    OK_RECT,
} ObjKind;

typedef struct {
    ObjKind kind;
    bool enabled;
    union {
        Rect rect;
    } as;
} Obj;
SP_STRUCT_ARR(ObjList, Obj);

typedef struct {
    ObjList objs;
    TaskList tasks;
    Id id_counter;
} Context;

#define UNIT_TO_PX 50
extern Arena arena;
extern Id _id_;

void spu_print_err(void *umka);
bool spu_call_fn(void *umka, const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes);
bool spu_get_tasks(void *umka, TaskList *tasks);
void sp_print_tasks(TaskList tl);
void spu_new_rect(UmkaStackSlot *p, UmkaStackSlot *r);
bool spu_content_preamble(Arena *arena, const char *filename, char **content);
Vector2 sp_from_dv2(DVector2 dv);
void spu_run_sequence(void *umka);

#endif // _SPAN_H_
