#include "arena.h"
#include <string.h>
#define ARENA_IMPLEMENTATION
#include "animator.h"
#define JIMP_IMPLEMENTATION
#include "jimp.h"
#include "ffmpeg.h"
#include "../raylib/include/raymath.h"


static anim_result _anim_obj_get_prop_by_name(
    anim_obj *obj, const char *prop_name, void **ptr
);
static anim_result _resolve_action_value_ptrs(anim_action_list *actions, anim_obj_list *objs);
static anim_result _parse_script(Jimp *j, anim_ctx *ctx);
anim_result anim_ctx_init(anim_ctx *ctx, const char *script_path) {
    *ctx = (anim_ctx) {0};
    ctx->script_path = script_path;
    // Get file size
    FILE *fp = fopen(script_path, "r");
    if (fp == NULL) {
        return AR_FILE_NOT_FOUND;
    }
    fseek(fp, 0L, SEEK_END);
    size_t fsz = ftell(fp);
    rewind(fp);

    // Read file content
    char *buffer = arena_alloc(&ctx->arena, fsz * sizeof(char) + 1);
    fread(buffer, fsz, 1, fp);
    buffer[fsz] = '\0';
    fclose(fp);

    // Parse json script
    Jimp jimp = {0};
    jimp_begin(&jimp, script_path, buffer, strlen(buffer));
    ctx->cfg = (anim_config) {
        .width = 1280,
        .height = 720,
        .output_path = "out.mov",
        .fps = 30
    };
    anim_result res = _parse_script(&jimp, ctx);
    ctx->rendering = false;
    ctx->complete = false;
    ctx->paused = true;
    ctx->duration = 0.0;
    ctx->act_idx = 0;
    _resolve_action_value_ptrs(&ctx->actions, &ctx->objs);

    return res;
}

void anim_ctx_deinit(anim_ctx *ctx) {
    arena_free(&ctx->arena);
}

void anim_ctx_reload(anim_ctx *ctx) {
    anim_ctx_deinit(ctx);
    anim_ctx_init(ctx, ctx->script_path);
}

void anim_obj_render(const anim_obj *obj) {
    switch (obj->kind) {
        case AOK_TEXT: {
            Font font = GetFontDefault();
            const anim_text *t = &obj->as.text;
            DrawTextEx(font, t->text, t->position, t->font_size, 2.0, t->color);
            break;
        }
        case AOK_RECT: {
            const anim_rect *r = &obj->as.rect;
            DrawRectangleV(r->position, r->size, r->color);
            break;
        }
    }
}

static bool anim_action_done(anim_action *a);
static void anim_action_step(anim_action *a, float dt);
void anim_ctx_step(anim_ctx *ctx, float dt) {
    if (ctx->actions.count == 0 || ctx->complete || ctx->paused) return;

    anim_action *a = &ctx->actions.items[ctx->act_idx];
    if (anim_action_done(a)) {
        ctx->act_idx++;
        if (ctx->act_idx >= ctx->actions.count) {
            ctx->complete = true;
            return;
        }
        a = &ctx->actions.items[ctx->act_idx];
    }

    anim_action_step(a, dt);
}

// ======================== STATIC FUNCTIONS ========================
static bool _parse_vec2(Jimp *j, Vector2 *v) {
    if (!jimp_array_begin(j)) return false;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    v->x = (float)j->number;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    v->y = (float)j->number;

    return jimp_array_end(j);
}

static bool _parse_color(Jimp *j, Color *c) {
    if (!jimp_array_begin(j)) return false;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    c->r = (unsigned char)j->number;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    c->g = (unsigned char)j->number;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    c->b = (unsigned char)j->number;

    jimp_array_item(j);
    if (!jimp_number(j)) return false;
    c->a = (unsigned char)j->number;

    return jimp_array_end(j);
}

static anim_result _parse_text(Jimp *j, Arena *a, anim_text *t) {
    const char *member = NULL;
    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "text", 4) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected a `string` for obj{text}.text\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
            t->text = arena_strdup(a, j->string);
        } else if (strncmp(member, "fontSize", 8) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for obj{text}.fontSize\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
            t->font_size = (float)j->number;
        } else if (strncmp(member, "position", 8) == 0) {
            if (!_parse_vec2(j, &t->position)) {
                jimp_diagf(j, "Expected a `Vector2` for obj{text}.position\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "color", 5) == 0) {
            if (!_parse_color(j, &t->color)) {
                jimp_diagf(j, "Expected a `Color` for obj{text}.color\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member in obj{text}: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    return AR_NONE;
}

static anim_result _parse_rect(Jimp *j, anim_rect *r) {
    const char *member = NULL;
    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "position", 8) == 0) {
            if (!_parse_vec2(j, &r->position)) {
                jimp_diagf(j, "Expected a `Vector2` for obj{rect}.position\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "size", 5) == 0) {
            if (!_parse_vec2(j, &r->size)) {
                jimp_diagf(j, "Expected a `Vector2` for obj{rect}.size\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "color", 5) == 0) {
            if (!_parse_color(j, &r->color)) {
                jimp_diagf(j, "Expected a `Color` for obj{rect}.color\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member in obj{rect}: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    return AR_NONE;
}

static anim_result _parse_obj(Jimp *j, Arena *a, anim_obj *obj) {
    if (!jimp_object_begin(j)) {
        jimp_diagf(j, "Expected the start of an object, '{', for an object in objs\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    const char *member = NULL;
    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "kind", 4) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected a `string` for obj.kind\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
            const char *kind = j->string;

            // Determine what kind of object this is
            if (strncmp(kind, "text", 4) == 0) {
                obj->kind = AOK_TEXT;
            } else if (strncmp(kind, "rect", 4) == 0) {
                obj->kind = AOK_RECT;
            } else {
                jimp_diagf(j, "Unknown kind of object: %s\n", kind);
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "obj_id", 6) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for obj.obj_id\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            obj->obj_id = (uint16_t)j->number;
        } else if (strncmp(member, "props", 5) == 0) {
            if (!jimp_object_begin(j)) {
                jimp_diagf(j, "Expected the start of an object, '{', for obj.props\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            switch (obj->kind) {
                case AOK_TEXT: {
                    anim_result res = _parse_text(j, a, &obj->as.text);
                    if (res) return res;
                    break;
                }
                case AOK_RECT: {
                    anim_result res = _parse_rect(j, &obj->as.rect);
                    if (res) return res;
                    break;
                }
            }

            if (!jimp_object_end(j)) {
                jimp_diagf(j, "Expected the end of an object, '}', for obj.props\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        }
    }

    if (!jimp_object_end(j)) {
        jimp_diagf(j, "Expected the end of an object, '}', for an object in objs\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    return AR_NONE;
}

static anim_result _parse_v2_interp(Jimp *j, Arena *a, anim_v2_interp *vi) {
    const char *member = NULL;

    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "start", 5) == 0) {
            if (!_parse_vec2(j, &vi->start)) {
                jimp_diagf(j, "Expected a `Vector2` for action{v2Interp}.start\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "end", 3) == 0) {
            if (!_parse_vec2(j, &vi->end)) {
                jimp_diagf(j, "Expected a `Vector2` for action{v2Interp}.end\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "obj_id", 6) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for action{v2Interp}.obj_id\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            vi->obj_id = (uint16_t)j->number;
        } else if (strncmp(member, "prop_name", 9) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected a `string` for action{v2Interp}.prop_name\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            vi->prop_name = arena_strdup(a, j->string);
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member in action{v2Interp}: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    return AR_NONE;
}

static anim_result _parse_clr_interp(Jimp *j, Arena *a, anim_clr_interp *ci) {
    const char *member = NULL;

    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "start", 5) == 0) {
            if (!_parse_color(j, &ci->start)) {
                jimp_diagf(j, "Expected a `Vector2` for action{clrInterp}.start\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "end", 3) == 0) {
            if (!_parse_color(j, &ci->end)) {
                jimp_diagf(j, "Expected a `Vector2` for action{clrInterp}.end\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "obj_id", 6) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for action{clrInterp}.obj_id\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            ci->obj_id = (uint16_t)j->number;
        } else if (strncmp(member, "prop_name", 9) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected a `string` for action{clrInterp}.prop_name\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            ci->prop_name = arena_strdup(a, j->string);
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member in action{clrInterp}: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    return AR_NONE;
}

static anim_result _parse_action(Jimp *j, Arena *a, anim_action *act) {
    if (!jimp_object_begin(j)) {
        jimp_diagf(j, "Expected the start of an object, '{', for actions\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    const char *member = NULL;
    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "kind", 4) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected a `string` for action.kind\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            const char *kind = j->string;
            if (strncmp(kind, "v2Interp", 8) == 0) {
                act->kind = AAK_V2_INTERP;
            } else if (strncmp(kind, "clrInterp", 8) == 0) {
                act->kind = AAK_CLR_INTERP;
            } else if (strncmp(kind, "wait", 8) == 0) {
                act->kind = AAK_WAIT;
            } else {
                jimp_diagf(j, "Unknown kind of action: %s\n", kind);
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "action_id", 9) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for action.action_id\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
            act->action_id = (uint16_t)j->number;
        } else if (strncmp(member, "duration", 8) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for action.action_id\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
            act->duration = (float)j->number;
        } else if (strncmp(member, "props", 5) == 0){
            if (!jimp_object_begin(j)) {
                jimp_diagf(j, "Expected the start of an object, '{', for action.props\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            switch (act->kind) {
                case AAK_V2_INTERP: {
                    anim_result res = _parse_v2_interp(j, a, &act->as.v2_interp);
                    if (res) return res;
                    break;
                }
                case AAK_CLR_INTERP: {
                    anim_result res = _parse_clr_interp(j, a, &act->as.clr_interp);
                    if (res) return res;
                    break;
                }
                case AAK_WAIT: break;
            }

            if (!jimp_object_end(j)) {
                jimp_diagf(j, "Expected the end of an object, '}', for action.props\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member in action: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    if (!jimp_object_end(j)) {
        jimp_diagf(j, "Expected the end of an object, '}', for action\n");
        return AR_FAILED_SCRIPT_PARSING;
    }
    return AR_NONE;
}

static anim_result _parse_config(Jimp *j, Arena *a, anim_config *cfg) {
    if (!jimp_object_begin(j)) {
        jimp_diagf(j, "Expected the start of an object, '{', for config\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    const char *member = NULL;
    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "width", 5) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for config.width\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            cfg->width = (int)j->number;
        } else if (strncmp(member, "height", 6) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for config.height\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            cfg->height = (int)j->number;
        } else if (strncmp(member, "outputPath", 10) == 0) {
            if (!jimp_string(j)) {
                jimp_diagf(j, "Expected an `string` for config.outputPath\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            cfg->output_path = arena_strdup(a, j->string);
        } else if (strncmp(member, "fps", 3) == 0) {
            if (!jimp_number(j)) {
                jimp_diagf(j, "Expected an `integer` for config.fps\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            cfg->fps = (int)j->number;
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member: %s\n", j->string);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    if (!jimp_object_end(j)) {
        jimp_diagf(j, "Expected the end of an object, '}', for config\n");
        return AR_FAILED_SCRIPT_PARSING;
    }
    return AR_NONE;
}

static anim_result _parse_script(Jimp *j, anim_ctx *ctx) {
    if (!jimp_object_begin(j)) {
        jimp_diagf(j, "Expected the start of an object, '{', for script\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    const char *member = NULL;

    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "config", 6) == 0) {
            anim_result res = _parse_config(j, &ctx->arena, &ctx->cfg);
            if (res) return res;
        } else if (strncmp(member, "objs", 4) == 0) {
            if (!jimp_array_begin(j)) {
                jimp_diagf(j, "Expected the start of an array, '[', for objs\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            while (jimp_array_item(j)) {
                anim_obj obj = {0};
                anim_result res = _parse_obj(j, &ctx->arena, &obj);
                if (res) return res;
                arena_da_append(&ctx->arena, &ctx->objs, obj);
            }

            if (!jimp_array_end(j)) {
                jimp_diagf(j, "Expected the end of an array, ']', for objs\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else if (strncmp(member, "actions", 7) == 0) {
            if (!jimp_array_begin(j)) {
                jimp_diagf(j, "Expected the start of an array, '[', for objs\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            while (jimp_array_item(j)) {
                anim_action action = {0};
                anim_result res = _parse_action(j, &ctx->arena, &action);
                ctx->total_duration += action.duration;
                if (res) return res;
                arena_da_append(&ctx->arena, &ctx->actions, action);
            }

            if (!jimp_array_end(j)) {
                jimp_diagf(j, "Expected the end of an array, ']', for objs\n");
                return AR_FAILED_SCRIPT_PARSING;
            }
        } else {
            jimp_unknown_member(j);
            jimp_diagf(j, "Unknown member of script: %s\n", member);
            return AR_FAILED_SCRIPT_PARSING;
        }
    }

    return AR_NONE;
}

static anim_result _anim_obj_get_prop_by_name(
    anim_obj *obj, const char *prop_name, void **ptr
) {
    switch (obj->kind) {
        case AOK_TEXT: {
            const anim_text *t = &obj->as.text;
            if (strncmp(prop_name, "position", 8) == 0) {
                *ptr = (void*) &t->position;
            } else if (strncmp(prop_name, "color", 5) == 0) {
                *ptr = (void*) &t->color;
            } else {
                return AR_UNKNOWN_PROPERTY_NAME;
            }
            break;
        }
        case AOK_RECT: {
            const anim_rect *r = &obj->as.rect;
            (void) r;
            // TODO: unimplemented
            return AR_UNKNOWN_PROPERTY_NAME;
        }
    }
    return AR_NONE;
}

static anim_result _resolve_action_value_ptrs(anim_action_list *actions, anim_obj_list *objs) {
    for (size_t i = 0; i < actions->count; i++) {
        anim_action *a = &actions->items[i];
        switch (a->kind) {
            case AAK_V2_INTERP: {
                anim_v2_interp *vi = &a->as.v2_interp;
                anim_obj *obj = &objs->items[vi->obj_id];
                _anim_obj_get_prop_by_name(obj, vi->prop_name, (void*)&vi->value);
                break;
            }
            case AAK_CLR_INTERP: {
                anim_clr_interp *ci = &a->as.clr_interp;
                anim_obj *obj = &objs->items[ci->obj_id];
                _anim_obj_get_prop_by_name(obj, ci->prop_name, (void*)&ci->value);
                break;
            }
            case AAK_WAIT: break;
        }
    }

    return AR_NONE;
}

static bool anim_action_done(anim_action *a) {
    return a->t >= a->duration;
}

static float rate_func(float t, float duration, InterpFunc func_type) {
    switch (func_type) {
        case IF_LINEAR:
            return t / duration;
        case IF_SINE:
            return -0.5 * cosf((PI / duration) * t) + 0.5;
    }

    assert(0 && "unreachable");
}

static void anim_action_step(anim_action *a, float dt) {
    float factor;
    if (anim_action_done(a)) {
        factor = 1.0;
    } else {
        factor = rate_func(a->t, a->duration, IF_SINE);
    }
    a->t += dt;

    switch (a->kind) {
        case AAK_V2_INTERP: {
            anim_v2_interp *vi = &a->as.v2_interp;
            *vi->value = Vector2Add(
                vi->start,
                Vector2Scale(Vector2Subtract(vi->end, vi->start), factor)
            );
            break;
        }
        case AAK_CLR_INTERP: {
            // Taken from raylib.h
            anim_clr_interp *ci = &a->as.clr_interp;
            *ci->value = (Color){
                (char)((1.0f - factor)*ci->start.r + factor*ci->end.r),
                (char)((1.0f - factor)*ci->start.g + factor*ci->end.g),
                (char)((1.0f - factor)*ci->start.b + factor*ci->end.b),
                (char)((1.0f - factor)*ci->start.a + factor*ci->end.a)
            };
            break;
        }
        case AAK_WAIT: break;
    }
}

