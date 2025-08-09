import std::io;
import std::math;
import rl;

faultdef UNKNOWN_PROPERTY_NAME;

struct Text
{
    String text;
    float font_size;
    Vector2 position;
    Color color;
}

enum ObjKind
{
    TEXT
}

union ObjUnion
{
    Text text;
}

struct Obj
{
    int obj_id;
    ObjKind kind;
    ObjUnion as;
}

enum InterpFunc
{
    LINEAR,
    SINE,
}

struct FltInterp
{
    float start;
    float *value;
    float end;
    float t;
    float duration;
}

struct Vector2Interp
{
    Vector2 start;
    Vector2 *value;
    Vector2 end;
    // NOTE: the properties below are only needed on parsing json script
    // to resolve the value ptr
    int obj_id;
    String prop_name;
}

struct ColorInterp
{
    Color start;
    Color *value;
    Color end;
    // NOTE: the properties below are only needed on parsing json script
    // to resolve the value ptr
    int obj_id;
    String prop_name;
}

enum ActionKind
{
    VECTOR2_INTERP,
    COLOR_INTERP
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

fn void Obj.rl_render(Obj *self)
{
    switch (self.kind) {
        case ObjKind.TEXT:
            Text *t = &self.as.text;
            rl::draw_text(t.text, (int)t.position.x, (int)t.position.y, (int)t.font_size, t.color);
    }
}

fn any? Obj.get_prop_by_name(Obj *self, String prop_name)
{
    switch (self.kind) {
        case ObjKind.TEXT:
            return self.as.text.get_prop_by_name(prop_name)!;
    }
}

fn void Action.step(Action *self, float dt)
{
    float factor;
    if (self.t > self.duration) {
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

fn void FltInterp.step(FltInterp *self, float factor) @dynamic
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        *self.value = self.start + factor * (self.end - self.start);
    }
}

