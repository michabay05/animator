import rl;

struct Circle
{
    Vector2 position;
    float radius;
    Color color;
}

struct Rectangle
{
    Vector2 position;
    Vector2 size;
    Color color;
}

interface Interpolable
{
    fn void step(float dt);
}

struct FltInterp (Interpolable)
{
    float start;
    float *value;
    float end;
    float t;
    float duration;
}

fn void FltInterp.step(FltInterp *self, float dt) @dynamic
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        *self.value = self.start + ((self.t / self.duration) * (self.end - self.start));
        self.t += dt;
    }
}

struct V2Interp (Interpolable)
{
    Vector2 start;
    Vector2 *value;
    Vector2 end;
    float t;
    float duration;
}

fn void V2Interp.step(V2Interp *self, float dt) @dynamic
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        *self.value = self.start + ((self.t / self.duration) * (self.end - self.start));
        self.t += dt;
    }
}

