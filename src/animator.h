#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "ffmpeg.h"

#include "../raylib/include/raylib.h"

typedef enum {
    AR_NONE,
    AR_FILE_NOT_FOUND,
    AR_FAILED_SCRIPT_PARSING,
    AR_UNKNOWN_PROPERTY_NAME,
    AR_FFMEG_ERROR
} anim_result;

typedef struct {
    int width;
    int height;
    const char *output_path;
    int fps;
} anim_config;

typedef struct {
    char *text;
    float font_size;
    Vector2 position;
    Color color;
} anim_text;

typedef struct {
    Vector2 position;
    Vector2 size;
    Color color;
} anim_rect;

typedef enum {
    AOK_TEXT,
    AOK_RECT,
} anim_obj_kind;

typedef struct {
    anim_obj_kind kind;
    uint16_t obj_id;
    union {
        anim_text text;
        anim_rect rect;
    } as;
} anim_obj;

typedef struct {
    anim_obj *items;
    size_t count;
    size_t capacity;
} anim_obj_list;

typedef enum {
    AAK_V2_INTERP,
    AAK_CLR_INTERP,
    AAK_WAIT
} anim_action_kind;

typedef enum {
    IF_LINEAR,
    IF_SINE,
} InterpFunc;

typedef struct {
    Vector2 start;
    Vector2 *value;
    Vector2 end;
    // Used only in the script parsing section only
    uint16_t obj_id;
    char *prop_name;
} anim_v2_interp;

typedef struct {
    Color start;
    Color *value;
    Color end;
    // Used only in the script parsing section only
    uint16_t obj_id;
    char *prop_name;
} anim_clr_interp;

typedef struct {
    anim_action_kind kind;
    uint16_t action_id;
    float t;
    float duration;
    union {
        anim_v2_interp v2_interp;
        anim_clr_interp clr_interp;
    } as;
} anim_action;

typedef struct {
    anim_action *items;
    size_t count;
    size_t capacity;
} anim_action_list;

typedef struct {
    const char *script_path;
    Arena arena;
    anim_config cfg;
    anim_obj_list objs;
    anim_action_list actions;

    bool paused;
    bool complete;
    size_t act_idx;

    bool rendering;
    float duration;
    float total_duration;
    FFMPEG *ffmpeg;
    RenderTexture2D rtex;
} anim_ctx;

anim_result anim_ctx_init(anim_ctx *ctx, const char *script_path);
void anim_obj_render(const anim_obj *obj);
void anim_ctx_step(anim_ctx *ctx, float dt);
void anim_ctx_reload(anim_ctx *ctx);
void anim_ctx_deinit(anim_ctx *ctx);
