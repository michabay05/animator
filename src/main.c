#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"
#define JIMP_IMPLEMENTATION
#include "jimp.h"

#include "../raylib/include/raylib.h"

typedef enum {
    AR_NONE,
    AR_FILE_NOT_FOUND,
    AR_FAILED_SCRIPT_PARSING
} anim_result;

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

typedef enum {
    AAK_V2_INTERP,
    AAK_CLR_INTERP,
    AAK_WAIT
} anim_action_kind;

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
    float duration;
    union {
        anim_v2_interp v2_interp;
        anim_clr_interp clr_interp;
    } as;
} anim_action;

typedef struct {
    Arena arena;
} anim_ctx;

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

static anim_result _parse_script(Jimp *j, Arena *a) {
    if (!jimp_object_begin(j)) {
        jimp_diagf(j, "Expected the start of an object, '{', for script\n");
        return AR_FAILED_SCRIPT_PARSING;
    }

    const char *member = NULL;

    while (jimp_object_member(j)) {
        member = j->string;
        if (strncmp(member, "config", 6) == 0) {
            printf("TODO: parse config...\n");
        } else if (strncmp(member, "objs", 4) == 0) {
            if (!jimp_array_begin(j)) {
                jimp_diagf(j, "Expected the start of an array, '[', for objs\n");
                return AR_FAILED_SCRIPT_PARSING;
            }

            while (jimp_array_item(j)) {
                anim_obj obj;
                anim_result res = _parse_obj(j, a, &obj);
                if (res) return res;
                // TODO: add them to a list here
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
                anim_action action;
                anim_result res = _parse_action(j, a, &action);
                if (res) return res;
                // TODO: add them to a list here
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

anim_result anim_ctx_init(anim_ctx *ctx, const char *script_path) {
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
    return _parse_script(&jimp, &ctx->arena);
}

int main(void) {
    anim_ctx ctx = {0};
    anim_ctx_init(&ctx, "./examples/test.json");

    arena_free(&ctx.arena);
    return 0;
}
