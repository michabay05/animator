import std::math;
import rl;

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
    int id;
    ObjKind kind;
    ObjUnion as;
}

struct Text
{
    String text;
    float font_size;
    Vector2 position;
    Color color;
}

/*
enum InterpFunc
{
    LINEAR,
    SINE,
}

interface Interpolable
{
    fn void step(float dt, InterpFunc func_type);
}

struct FltInterp (Interpolable)
{
    float start;
    float *value;
    float end;
    float t;
    float duration;
}

struct Vec2Interp (Interpolable)
{
    Vector2 start;
    Vector2 *value;
    Vector2 end;
    float t;
    float duration;
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

fn void FltInterp.step(FltInterp *self, float dt, InterpFunc func_type) @dynamic
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        float v = rate_func(self.t, self.duration, func_type);
        *self.value = self.start + v * (self.end - self.start);
        self.t += dt;
    }
}

fn void Vec2Interp.step(Vec2Interp *self, float dt, InterpFunc func_type) @dynamic
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        float v = rate_func(self.t, self.duration, func_type);
        *self.value = self.start + v * (self.end - self.start);
        self.t += dt;
    }
}
*/

