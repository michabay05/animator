import std::collections::list;
import std::io;
import std::io::file;
import std::math;
import std::core::mem;

import ffmpeg;
import jimp;
import rl;

faultdef UNKNOWN_PROPERTY_NAME, FAILED_SCRIPT_PARSING;

struct Config
{
    int width;
    int height;
    ZString output_path;
    int fps;
}

struct Text
{
    ZString text;
    float font_size;
    Vector2 position;
    Color color;
}

fn any? Text.get_prop_by_name(Text *self, String prop_name)
{
    switch (prop_name) {
        case "position":
            return &self.position;
        case "color":
            return &self.color;
        default:
            return UNKNOWN_PROPERTY_NAME?;
    }
}

struct Rect
{
    Vector2 position;
    Vector2 size;
    Color color;
}

fn any? Rect.get_prop_by_name(Rect *self, String prop_name)
{
    switch (prop_name) {
        default:
            return UNKNOWN_PROPERTY_NAME?;
    }
}

enum ObjKind
{
    TEXT,
    RECT
}

union ObjUnion
{
    Text text;
    Rect rect;
}

struct Obj
{
    int obj_id;
    ObjKind kind;
    ObjUnion as;
}

fn any? Obj.get_prop_by_name(Obj *self, String prop_name)
{
    switch (self.kind) {
        case ObjKind.TEXT:
            return self.as.text.get_prop_by_name(prop_name)!;
        case ObjKind.RECT:
            return self.as.rect.get_prop_by_name(prop_name)!;
    }
}

fn void Obj.rl_render(Obj *self)
{
    switch (self.kind) {
        case ObjKind.TEXT:
            Text *t = &self.as.text;
            rl::draw_text(t.text, (int)t.position.x, (int)t.position.y, (int)t.font_size, t.color);
        case ObjKind.RECT:
            Rect *r = &self.as.rect;
            rl::draw_rectangle_v(r.position, r.size, r.color);
    }
}

enum InterpFunc
{
    LINEAR,
    SINE,
}

struct Vector2Interp
{
    Vector2 start;
    Vector2 *value;
    Vector2 end;
    // NOTE: the properties below are only needed on parsing json script
    // to resolve the value ptr
    int obj_id;
    ZString prop_name;
}

struct ColorInterp
{
    Color start;
    Color *value;
    Color end;
    // NOTE: the properties below are only needed on parsing json script
    // to resolve the value ptr
    int obj_id;
    ZString prop_name;
}

enum ActionKind
{
    VECTOR2_INTERP,
    COLOR_INTERP,
    WAIT,
}

union ActionUnion
{
    Vector2Interp v2_interp;
    ColorInterp clr_interp;
}

struct Action
{
    ActionKind kind;
    ActionUnion as;
    int action_id;
    float t;
    float duration;
}

fn bool Action.done(Action *self) => self.t > self.duration;

fn void Action.step(Action *self, float dt)
{
    float factor;
    if (self.done()) {
        factor = 1.0;
    } else {
        factor = rate_func(self.t, self.duration, InterpFunc.SINE);
    }
    self.t += dt;
    switch (self.kind) {
        case ActionKind.VECTOR2_INTERP:
            Vector2Interp *vi = &self.as.v2_interp;
            *vi.value = vi.start + factor * (vi.end - vi.start);
        case ActionKind.COLOR_INTERP:
            ColorInterp *ci = &self.as.clr_interp;
            *ci.value = {
                (char)((1.0f - factor)*ci.start.r + factor*ci.end.r),
                (char)((1.0f - factor)*ci.start.g + factor*ci.end.g),
                (char)((1.0f - factor)*ci.start.b + factor*ci.end.b),
                (char)((1.0f - factor)*ci.start.a + factor*ci.end.a)
            };
        case ActionKind.WAIT: {}
    }
}

fn float rate_func(float t, float duration, InterpFunc func_type)
{
    const float PI = 3.141592653589;
    switch (func_type) {
        case InterpFunc.LINEAR:
            return t / duration;
        case InterpFunc.SINE:
            return -0.5 * math::cos((PI / duration) * t) + 0.5;
    }
}

struct Video
{
    // Parsing
    String script_path;
    Config cfg;
    List{Obj} objs;
    List{Action} actions;

    usz act_idx;
    bool paused;
    bool complete;

    bool rendering;
    Ffmpeg *ffmpeg;
    float duration;
    float max_duration;
    RenderTexture2D rtex;
}

fn bool parse_str(Jimp *jimp, ZString *str) @local
{
    if (!jimp::string(jimp)) return false;
    *str = ((ZString)jimp.string).str_view().zstr_tcopy();
    return true;
}

fn bool parse_vec2(Jimp *jimp, Vector2 *v) @local
{
    if (!jimp::array_begin(jimp)) return false;
    int i = 0;
    while (jimp::array_item(jimp)) {
        if (i >= rl::Vector2.len) return false;
        if (!jimp::number(jimp)) return false;
        (*v)[i++] = (float)jimp.number;
    }
    return jimp::array_end(jimp);
}

fn bool parse_color(Jimp *jimp, Color *c) @local
{
    if (!jimp::array_begin(jimp)) return false;
    int i = 0;
    while (jimp::array_item(jimp)) {
        if (i >= rl::Color.len) return false;
        if (!jimp::number(jimp)) return false;
        (*c)[i++] = (char)jimp.number;
    }
    return jimp::array_end(jimp);
}

fn void? parse_text(Jimp *jimp, Text *t) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "text") {
            if (!parse_str(jimp, &t.text)) {
                jimp::diagf(jimp, "Expected `string` for obj{text}.text\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "fontSize") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected `string` for obj{text}.fontSize\n");
                return FAILED_SCRIPT_PARSING?;
            }
            t.font_size = (float)jimp.number;
        } else if (member == "position") {
            if (!parse_vec2(jimp, &t.position)) {
                jimp::diagf(jimp, "Expected `string` for obj{text}.position\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "color") {
            if (!parse_color(jimp, &t.color)) {
                jimp::diagf(jimp, "Expected `color` for obj{text}.color\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in obj{text}: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }
}

fn void? parse_rect(Jimp *jimp, Rect *r) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "position") {
            if (!parse_vec2(jimp, &r.position)) {
                jimp::diagf(jimp, "Expected `string` for obj{rect}.position\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "size") {
            if (!parse_vec2(jimp, &r.size)) {
                jimp::diagf(jimp, "Expected `string` for obj{rect}.size\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "color") {
            if (!parse_color(jimp, &r.color)) {
                jimp::diagf(jimp, "Expected `color` for obj{rect}.color\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in obj{rect}: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }
}

fn void? parse_obj(Jimp *jimp, Obj *obj) @local
{
    if (!jimp::object_begin(jimp)) {
        jimp::diagf(jimp, "Expected the start of an object, '{', for object 'obj'\n");
        return FAILED_SCRIPT_PARSING?;
    }

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "kind") {
            ZString kind;
            if (!parse_str(jimp, &kind)) {
                jimp::diagf(jimp, "Expected a `string` for obj.kind\n");
                return FAILED_SCRIPT_PARSING?;
            }
            switch (kind.str_view()) {
                case "text":
                    obj.kind = ObjKind.TEXT;
                case "rect":
                    obj.kind = ObjKind.RECT;
                default:
                    jimp::diagf(jimp, "Unknown kind of object: %s\n", kind);
                    return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `integer` for obj.obj_id\n");
                return FAILED_SCRIPT_PARSING?;
            }
            obj.obj_id = (int)jimp.number;
        } else if (member == "props") {
            if (!jimp::object_begin(jimp)) {
                jimp::diagf(jimp, "Expected the start of an object, '{', for obj.props\n");
                return FAILED_SCRIPT_PARSING?;
            }

            switch (obj.kind) {
                case ObjKind.TEXT:
                    parse_text(jimp, &obj.as.text)!;
                case ObjKind.RECT:
                    parse_rect(jimp, &obj.as.rect)!;
            }

            if (!jimp::object_end(jimp)) {
                jimp::diagf(jimp, "Expected the end of an object, '}', for obj.props\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in object 'obj': %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }

    if (!jimp::object_end(jimp)) {
        jimp::diagf(jimp, "Expected the end of an object, '}', for object 'obj'\n");
        return FAILED_SCRIPT_PARSING?;
    }
}

fn void? parse_v2_interp(Jimp *jimp, Vector2Interp *vi) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "start") {
            if (!parse_vec2(jimp, &vi.start)) {
                jimp::diagf(jimp, "Expected a `Vector2` for action{v2Interp}.start\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "end") {
            if (!parse_vec2(jimp, &vi.end)) {
                jimp::diagf(jimp, "Expected a `Vector2` for action{v2Interp}.end\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `int` for action{v2Interp}.obj_id\n");
                return FAILED_SCRIPT_PARSING?;
            }
            vi.obj_id = (int)jimp.number;
        } else if (member == "prop_name") {
            if (!parse_str(jimp, &vi.prop_name)) {
                jimp::diagf(jimp, "Expected an `string` for action{v2Interp}.prop_name\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in action{v2Interp}: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }
}

fn void? parse_clr_interp(Jimp *jimp, ColorInterp *ci) @local
{
    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "start") {
            if (!parse_color(jimp, &ci.start)) {
                jimp::diagf(jimp, "Expected a `Color` for action{clrInterp}.start\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "end") {
            if (!parse_color(jimp, &ci.end)) {
                jimp::diagf(jimp, "Expected a `Color` for action{clrInterp}.end\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "obj_id") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected a `Color` for action{clrInterp}.obj_id\n");
                return FAILED_SCRIPT_PARSING?;
            }
            ci.obj_id = (int)jimp.number;
        } else if (member == "prop_name") {
            if (!parse_str(jimp, &ci.prop_name)) {
                jimp::diagf(jimp, "Expected an `string` for action{clrInterp}.prop_name\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in action{clrInterp}: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }
}

fn void? parse_action(Jimp *jimp, Action *action) @local
{
    if (!jimp::object_begin(jimp)) {
        jimp::diagf(jimp, "Expected the start of an object, '{', for actions\n");
        return FAILED_SCRIPT_PARSING?;
    }

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "kind") {
            ZString kind;
            if (!parse_str(jimp, &kind)) {
                jimp::diagf(jimp, "Expected `string` for action.kind\n");
                return FAILED_SCRIPT_PARSING?;
            }
            switch (kind.str_view()) {
                case "v2Interp":
                    action.kind = ActionKind.VECTOR2_INTERP;
                case "clrInterp":
                    action.kind = ActionKind.COLOR_INTERP;
                case "wait":
                    action.kind = ActionKind.WAIT;
                default:
                    jimp::diagf(jimp, "Unknown kind of action: %s\n", kind);
                    return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "action_id") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `integer` for action.action_id\n");
                return FAILED_SCRIPT_PARSING?;
            }
            action.action_id = (int)jimp.number;
        } else if (member == "duration") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `float` for action.duration\n");
                return FAILED_SCRIPT_PARSING?;
            }
            action.duration = (float)jimp.number;
        } else if (member == "props") {
            if (!jimp::object_begin(jimp)) {
                jimp::diagf(jimp, "Expected the start of an object, '{', for action.props\n");
                return FAILED_SCRIPT_PARSING?;
            }

            switch (action.kind) {
                case ActionKind.VECTOR2_INTERP:
                    parse_v2_interp(jimp, &action.as.v2_interp)!;
                case ActionKind.COLOR_INTERP:
                    parse_clr_interp(jimp, &action.as.clr_interp)!;
                case ActionKind.WAIT: {}
            }

            if (!jimp::object_end(jimp)) {
                jimp::diagf(jimp, "Expected the end of an object, '}', for action.props\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member in action: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }

    if (!jimp::object_end(jimp)) {
        jimp::diagf(jimp, "Expected the end of an object, '}', for actions\n");
        return FAILED_SCRIPT_PARSING?;
    }
}

fn void? resolve_action_value_ptrs(Action[] actions, Obj[] objs) @local
{
    foreach (&action : actions) {
        switch (action.kind) {
            case ActionKind.VECTOR2_INTERP:
                Vector2Interp* vi = &action.as.v2_interp;
                Obj *obj = &objs[vi.obj_id];
                any? value_ptr = obj.get_prop_by_name(vi.prop_name.str_view())!;
                vi.value = anycast(value_ptr, rl::Vector2)!;
            case ActionKind.COLOR_INTERP:
                ColorInterp* ci = &action.as.clr_interp;
                Obj *obj = &objs[ci.obj_id];
                any? value_ptr = obj.get_prop_by_name(ci.prop_name.str_view())!;
                ci.value = anycast(value_ptr, rl::Color)!;
            case ActionKind.WAIT: {}
        }
    }
}

fn void? parse_config(Jimp *jimp, Config *cfg) @local
{
    if (!jimp::object_begin(jimp)) {
        jimp::diagf(jimp, "Expected the start of an object, '{', for config\n");
        return FAILED_SCRIPT_PARSING?;
    }

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "width") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `integer` for config.width\n");
                return FAILED_SCRIPT_PARSING?;
            }
            cfg.width = (int)jimp.number;
        } else if (member == "height") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `integer` for config.height\n");
                return FAILED_SCRIPT_PARSING?;
            }
            cfg.height = (int)jimp.number;
        } else if (member == "outputPath") {
            if (!parse_str(jimp, &cfg.output_path)) {
                jimp::diagf(jimp, "Expected an `string` for config.outputPath\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "fps") {
            if (!jimp::number(jimp)) {
                jimp::diagf(jimp, "Expected an `integer` for config.outputPath\n");
                return FAILED_SCRIPT_PARSING?;
            }
            cfg.fps = (int)jimp.number;
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member: %s\n", (ZString)jimp.string);
            return FAILED_SCRIPT_PARSING?;
        }
    }

    if (!jimp::object_end(jimp)) {
        jimp::diagf(jimp, "Expected the end of an object, '}', for config\n");
        return FAILED_SCRIPT_PARSING?;
    }
}

fn void? parse_script(Jimp *jimp, Video *v) @local
{
    if (!jimp::object_begin(jimp)) {
        jimp::diagf(jimp, "Expected the start of an object, '{', for script\n");
        return FAILED_SCRIPT_PARSING?;
    }

    ZString member;
    while (jimp::object_member(jimp)) {
        member = (ZString)jimp.string;
        if (member == "config") {
            parse_config(jimp, &v.cfg)!;
        } else if (member == "objs") {
            if (!jimp::array_begin(jimp)) {
                jimp::diagf(jimp, "Expected the start of an array, '[', for objs\n");
                return FAILED_SCRIPT_PARSING?;
            }

            while (jimp::array_item(jimp)) {
                Obj obj;
                parse_obj(jimp, &obj)!;
                v.objs.push(obj);
            }

            if (!jimp::array_end(jimp)) {
                jimp::diagf(jimp, "Expected the end of an array, ']', for objs\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else if (member == "actions") {
            if (!jimp::array_begin(jimp)) {
                jimp::diagf(jimp, "Expected the start of an array, '[', for actions\n");
                return FAILED_SCRIPT_PARSING?;
            }

            while (jimp::array_item(jimp)) {
                Action action;
                parse_action(jimp, &action)!;
                v.actions.push(action);
                v.max_duration += action.duration;
            }

            if (!jimp::array_end(jimp)) {
                jimp::diagf(jimp, "Expected the end of an array, ']', for actions\n");
                return FAILED_SCRIPT_PARSING?;
            }
        } else {
            jimp::unknown_member(jimp);
            jimp::diagf(jimp, "Unknown member of script: %s\n", member);
            return FAILED_SCRIPT_PARSING?;
        }
    }

    if (!jimp::object_end(jimp)) {
        jimp::diagf(jimp, "Expected the end of an object, '}', for script\n");
        return FAILED_SCRIPT_PARSING?;
    }
}

fn void? Video.init(Video *self, String script_path)
{
    self.script_path = script_path;
    // TODO: replace this to make the buffer depend on the size of the file
    File f = file::open(script_path, "r")!;
    defer (void)f.close();
    usz fsz = file::get_size(script_path)!;
    char[] buffer = mem::talloc_array(char, fsz);
    f.read(buffer[..])!;
    ZString content = (ZString)buffer[..];

    Jimp jimp;
    jimp::begin(&jimp, script_path, content, content.len());

    // Set some default values for the config
    self.cfg = {
        .width = 800,
        .height = 600,
        .output_path = "out.mov",
        .fps = 30,
    };

    parse_script(&jimp, self)!;

    self.rendering = false;
    self.complete = false;
    self.paused = true;
    self.ffmpeg = null;
    self.duration = 0.0;
    self.act_idx = 0;

    resolve_action_value_ptrs(self.actions.array_view(), self.objs.array_view())!;
}

fn bool Video.can_render(Video *self) => self.max_duration > 1;

fn void Video.deinit(Video *self)
{
    rl::unload_render_texture(self.rtex);
    self.objs.free();
    self.actions.free();
}

fn void? Video.reload_script(Video *self)
{
    self.deinit();
    self.init(self.script_path)!;
}

fn void Video.step(Video *self, float dt)
{
    if (self.actions.len() == 0 || self.complete || self.paused) return;

    Action *a = &self.actions[self.act_idx];
    if (a.done()) {
        self.act_idx += 1;
        if (self.act_idx >= self.actions.len()) {
            self.complete = true;
            return;
        }
        a = &self.actions[self.act_idx];
    }
    a.step(dt);
}

fn void Video.render(Video *self)
{
    rl::clear_background(rl::get_color(0x181818FF));
    foreach (&obj : self.objs) {
        obj.rl_render();
    }
}
