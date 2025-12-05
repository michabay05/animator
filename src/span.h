#ifndef _SPAN_H_
#define _SPAN_H_

#include <stdint.h>
#include "ffmpeg.h"
#include "raylib.h"
#include "arena.h"
#include "umka_api.h"


#define SP_LEN(arr) ((int)(sizeof(arr) / sizeof(arr[0])))
#define SP_ASSERT assert
#define SP_UNUSED(x) ((void)x)
#define SP_UNIMPLEMENTED(message)                                                                 \
    do {                                                                                          \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, message);                 \
        abort();                                                                                  \
    } while (0)
#define SP_UNREACHABLEF(message, ...)                                                             \
    do {                                                                                          \
        fprintf(stderr, "%s:%d: UNREACHABLE: " message "\n", __FILE__, __LINE__, __VA_ARGS__);    \
        abort();                                                                                  \
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
    int x, y;
} IVector2;

typedef struct {
    f64 x, y;
} DVector2;

// NOTE: Don't rearrange order without modifying Umka enum
typedef enum {
    AK_Enable,
    AK_Wait,
    AK_Fade,
    AK_Move,
} ActionKind;

typedef struct {
    Color start;
    Color end;
} FadeData;

typedef struct {
    DVector2 start;
    DVector2 end;
} MoveData;

typedef struct {
    Id obj_id;
    ActionKind kind;
    f64 delay;
    union {
        FadeData fade;
        MoveData move;
    } args;
} Action;
SP_STRUCT_ARR(ActionList, Action);

typedef struct {
    const char *name;
    UmkaExternFunc func;
} UmkaFunc;

typedef struct {
    ActionList actions;
    f64 duration;
} Task;
SP_STRUCT_ARR(TaskList, Task);

typedef struct {
    DVector2 position;
    DVector2 size;
    Color color;
} Rect;

typedef struct {
    const char *str;
    DVector2 position;
    Vector2 norm_coords;
    f32 font_size;
    Color color;
} Text;

SP_STRUCT_ARR(PointList, Vector2);
// NOTE: Personally, I prefer that "children" don't possess any knowledge of
// their environment and who their "parent" is. But, for this one case, I
// will try to ignore that and give curves knowledge of their "parent": axes
typedef struct {
    Id axes_id;
    PointList pts;
    Color color;
} Curve;

typedef struct {
    f64 xmin, xmax, ymin, ymax;
    Rectangle box;
    Vector2 origin_pos, coord_size, center_coord;
} Axes;

typedef enum {
    OK_RECT,
    OK_TEXT,
    OK_AXES,
    OK_CURVE,
} ObjKind;

typedef struct {
    Id id;
    ObjKind kind;
    bool enabled;
    union {
        Rect rect;
        Text text;
        Axes axes;
        Curve curve;
    } as;
} Obj;
SP_STRUCT_ARR(ObjList, Obj);

typedef enum {
    EM_Linear,
    EM_Sine,
} EaseMode;

typedef enum {
    RM_Preview,
    RM_Output,
} RenderMode;

typedef struct {
    void *umka;

    // NOTE: this object list contains the original, unmodified state of the objects
    ObjList orig_objs;
    ObjList objs;

    TaskList tasks;
    Id id_counter;
    EaseMode easing;

    int preamble_lines;
    int current;
    f32 t;
    bool paused, quit;

    // NOTE: preview window resolution, output video resolution
    IVector2 pres, vres;
    Camera2D cam;
    int fps;
    RenderMode render_mode;
    RenderTexture rtex;
    // NOTE: I'm not sure how to name this...essentially, if it is >= 1, dt
    // is multiplied by it. If it's < -1, then dt is divided by it. It can't be zero.
    int dt_mul;

    FFMPEG *ffmpeg;
} Context;

#define UNIT_TO_PX 50
#define SCENE_OBJ ((Id)-1)

extern Arena arena;
extern Context ctx;

// TODO: all of these function do not need to be here; some should just be
// static and in the `span.c` file.
bool spc_init(const char *filename, RenderMode mode);
bool spc_umka_init(const char *filename);
void spc_renderer_init(RenderMode mode);
void spc_deinit(void);
void spc_update(f32 dt);
void spc_render(void);
Id spc_next_id(void);
void spc_print_tasks(TaskList tl);
void spc_new_task(f64 duration);
void spc_add_action(Action action);
bool spc_get_obj(Id id, Obj **obj);
void spc_clear_tasks(void);
void spc_reset(void);
Obj spo_rect(DVector2 pos, DVector2 size, Color color);
void spo_get_pos(Obj *obj, DVector2 **pos);
void spo_get_color(Obj *obj, Color **color);
void spo_render(Obj obj);
Action spo_enable(Id obj_id);
void spu_run_sequence(void);
void spu_print_err(void);
bool spu_call_fn(const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes);
bool spu_content_w_preamble(const char *filename, char **content);
void spuo_rect(UmkaStackSlot *p, UmkaStackSlot *r);
void spuo_text(UmkaStackSlot *p, UmkaStackSlot *r);
void spuo_axes(UmkaStackSlot *p, UmkaStackSlot *r);
void spuo_curve(UmkaStackSlot *p, UmkaStackSlot *r);
void spuo_enable(UmkaStackSlot *p, UmkaStackSlot *r);
void spu_fade_in(UmkaStackSlot *p, UmkaStackSlot *r);
void spu_fade_out(UmkaStackSlot *p, UmkaStackSlot *r);
void spu_move(UmkaStackSlot *p, UmkaStackSlot *r);
void spu_wait(UmkaStackSlot *p, UmkaStackSlot *r);
void spu_play(UmkaStackSlot *p, UmkaStackSlot *r);
void spa_interp(Action action, void **value, f32 factor);
f32 sp_easing(f32 t, f32 duration);
Vector2 spv_dtof(DVector2 dv);
DVector2 spv_ftod(Vector2 v);
Vector2 spv_itof(IVector2 iv);
DVector2 spv_lerpd(DVector2 start, DVector2 end, f64 factor);

#endif // _SPAN_H_
