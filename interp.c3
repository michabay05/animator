module interp{Type};

struct Interp
{
    Type start;
    Type *value;
    Type end;
    float t;
    float duration;
}

fn void Interp.step(Interp *self, float dt)
{
    if (self.t > self.duration) {
        *self.value = self.end;
    } else {
        *self.value = self.start + ((self.t / self.duration) * (self.end - self.start));
        self.t += dt;
    }
}
