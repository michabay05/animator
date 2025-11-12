use std::f32::consts::PI;

use crate::acode::{self, Instruction, ObjInfo};
use crate::rl::{self, Camera2D, Color, ConfigFlags, Font, Rectangle, TraceLogLevel, Vector2};

const UNIT_TO_PX: f32 = 50.0;
// So, 1 unit = UNIT_TO_PX px

#[derive(Debug)]
struct RectInfo {
    position: Vec2,
    size: Vec2,
    color: Color,
    origin: Vec2,
}

impl Default for RectInfo {
    fn default() -> Self {
        let pos = Vec2::Relative(0.0, 0.0);
        let size = Vec2::Relative(1.0, 1.0);
        let origin = Vector2::add(pos.vec2(), Vector2::scale(size.vec2(), 0.5));
        Self {
            position: pos,
            size,
            color: Color::WHITE,
            origin: Vec2::Relative(origin.x, origin.y),
        }
    }
}

#[derive(Debug)]
struct TextInfo {
    text: String,
    font_size: f32,
    position: Vec2,
    color: Color,
    size: Vec2,
    origin: Vec2,
}

impl TextInfo {
    fn default(s: &str) -> Self {
        let pos = Vec2::Relative(0.0, 0.0);
        let font_size: f32 = 30.0;
        let sz = Vector2::scale(
            rl::measure_text_ex(Font::default(), s, font_size, 0.0),
            1.0 / UNIT_TO_PX,
        );
        let origin = Vector2::add(pos.vec2(), Vector2::scale(sz, 0.5));
        Self {
            text: s.to_string(),
            font_size,
            position: pos,
            color: Color::WHITE,
            size: Vec2::Relative(sz.x, sz.y),
            origin: Vec2::Relative(origin.x, origin.y),
        }
    }
}

#[derive(Debug)]
enum ObjKind {
    Rect(RectInfo),
    Text(TextInfo),
}

#[derive(Debug)]
struct Obj {
    kind: ObjKind,
    enabled: bool,
}

impl Obj {
    fn rect() -> Self {
        Self {
            kind: ObjKind::Rect(RectInfo::default()),
            enabled: false,
        }
    }

    fn text(s: &str) -> Self {
        Self {
            kind: ObjKind::Text(TextInfo::default(s)),
            enabled: false,
        }
    }
}

struct Context {
    objs: Vec<Obj>,
    setup_insts: Vec<Instruction>,
    loop_insts: Vec<Instruction>,

    // Info about current instruction
    bounds: Option<InterpBounds>,
    inst_ind: usize,
    t: f32,

    camera: Camera2D,
    fps: i32,
}

impl Context {
    fn from_path(filepath: &str, dimension: (i32, i32)) -> Self {
        let (setup_insts, loop_insts) = acode::parse(filepath);
        let mut this = Self {
            objs: vec![],
            setup_insts,
            loop_insts,
            bounds: None,
            inst_ind: 0,
            t: 0.0,

            fps: 60,
            camera: Camera2D {
                offset: Vec2::Absolute(0.5 * dimension.0 as f32, 0.5 * dimension.1 as f32)
                    .absolute(),
                target: Vec2::Absolute(0.0, 0.0).absolute(),
                rotation: 0.0,
                zoom: 1.0,
            },
        };
        this.setup();
        this
    }

    fn setup(&mut self) {
        rl::set_target_fps(self.fps);
        for inst in &self.setup_insts {
            match inst {
                Instruction::Create(info) => match info {
                    ObjInfo::Rect => self.objs.push(Obj::rect()),
                    ObjInfo::Text(s) => self.objs.push(Obj::text(&s)),
                },
                _ => unreachable!("Unknown instruction in init phase: '{:?}'", inst),
            }
        }

        // dbg!(&self.objs);
    }

    fn move_on(&mut self) {
        // Reset and prepare for next instruction
        self.inst_ind += 1;
        self.t = 0.0;
        self.bounds = None;
    }

    fn update(&mut self, dt: f32) {
        if self.inst_ind >= self.loop_insts.len() {
            return;
        }

        let inst = &self.loop_insts[self.inst_ind];
        match inst {
            Instruction::Grow(ind, duration) => {
                // Grow can enable an object if it is not already
                let obj = &mut self.objs[*ind];
                if !obj.enabled {
                    obj.enabled = true;
                }
                match &mut obj.kind {
                    ObjKind::Rect(ri) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Vec2(
                                Vec2::Relative(0.0, 0.0),
                                Vec2::Relative(1.0, 1.0),
                            ));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_vec2();
                        ri.size = ri.size.interp(start, end, self.t, *duration);
                    }
                    ObjKind::Text(ti) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Scalar(0.0, ti.font_size));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_f32();
                        ti.font_size = ti.font_size.interp(start, end, self.t, *duration);
                    }
                    _ => unreachable!("Idk how to 'Grow' {:?}", obj),
                }

                self.t += dt;
                if self.t >= *duration {
                    self.move_on();
                }
            }

            Instruction::Move(ind, target, duration) => {
                // Move can not enable an object if it is not already
                let obj = &mut self.objs[*ind];
                match &mut obj.kind {
                    ObjKind::Rect(ri) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Vec2(
                                ri.position,
                                Vec2::Relative(target.x, target.y),
                            ));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_vec2();
                        ri.position = ri.position.interp(start, end, self.t, *duration);
                    }
                    ObjKind::Text(ti) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Vec2(
                                ti.position,
                                Vec2::Relative(target.x, target.y),
                            ));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_vec2();
                        ti.position = ti.position.interp(start, end, self.t, *duration);
                    }
                    _ => unreachable!("Idk how to 'Move' {:?}", obj),
                }
                self.t += dt;
                if self.t >= *duration {
                    self.move_on();
                }
            }

            Instruction::Fade(ind, target, duration) => {
                // Fade can enable an object if it is not already
                let obj = &mut self.objs[*ind];
                if !obj.enabled {
                    obj.enabled = true;
                }
                match &mut obj.kind {
                    ObjKind::Rect(ri) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Clr(ri.color, *target));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_clr();
                        ri.color = ri.color.interp(start, end, self.t, *duration);
                    }
                    ObjKind::Text(ti) => {
                        if self.bounds.is_none() {
                            self.bounds = Some(InterpBounds::Clr(ti.color, *target));
                        }

                        let (start, end) = self.bounds.unwrap().unwrap_clr();
                        ti.color = ti.color.interp(start, end, self.t, *duration);
                    }
                    _ => unreachable!("Idk how to 'Fade' {:?}", obj),
                }
                self.t += dt;
                if self.t >= *duration {
                    self.move_on();
                }
            }

            Instruction::Wait(duration) => {
                self.t += dt;
                if self.t >= *duration {
                    self.move_on();
                }
            }
            _ => unreachable!("Unsure what to do with: {:?}", inst),
        }
    }

    fn render(&self) {
        rl::begin_mode_2d(self.camera.clone());
        rl::clear_background(Color::BLACK);

        for obj in &self.objs {
            if !obj.enabled {
                continue;
            }

            match &obj.kind {
                ObjKind::Rect(ri) => {
                    rl::draw_rectangle_pro(
                        Rectangle::from_vec2(ri.position.absolute(), ri.size.absolute()),
                        ri.origin.absolute(),
                        0.0,
                        ri.color,
                    );
                }
                ObjKind::Text(ti) => {
                    rl::draw_text_pro(
                        Font::default(),
                        ti.text.as_str(),
                        ti.position.absolute(),
                        ti.origin.absolute(),
                        0.0,
                        ti.font_size,
                        0.0,
                        ti.color,
                    );
                }
            }
        }
    }
}

pub fn engine_expr() {
    rl::set_config_flags(&[ConfigFlags::Msaa4xHint, ConfigFlags::WindowResizable]);
    rl::set_trace_log_level(TraceLogLevel::WARNING);
    let factor = 300;
    let scr_sz: (i32, i32) = (4 * factor, 3 * factor);
    rl::init_window(scr_sz.0, scr_sz.1, "anim");

    let mut ctx = Context::from_path("acodes/simple.ac", scr_sz);

    while !rl::window_should_close() {
        ctx.update(rl::get_frame_time());

        rl::begin_drawing();
        ctx.render();
        rl::end_drawing();
    }

    rl::close_window();
}

fn rate_func(t: f32, duration: f32) -> f32 {
    if t >= duration {
        1.0
    } else {
        // >> Linear
        t / duration

        // >> Sinusoidal
        // -0.5 * ((PI / duration) * t).cos() + 0.5
    }
}

#[derive(Clone, Copy, Debug)]
enum Vec2 {
    Relative(f32, f32),
    Absolute(f32, f32),
}

impl Vec2 {
    fn vec2(self) -> Vector2 {
        match self {
            Self::Relative(x, y) => Vector2::new(x, y),
            Self::Absolute(x, y) => Vector2::new(x, y),
        }
    }

    fn absolute(self) -> Vector2 {
        match self {
            Self::Relative(x, y) => Vector2::scale(Vector2::new(x, y), UNIT_TO_PX),
            Self::Absolute(x, y) => Vector2::new(x, y),
        }
    }

    fn interp(self, start: Vector2, end: Vector2, t: f32, duration: f32) -> Self {
        let v = self.vec2().interp(start, end, t, duration);
        match self {
            Self::Relative(_, _) => Self::Relative(v.x, v.y),
            Self::Absolute(_, _) => Self::Absolute(v.x, v.y),
        }
    }
}

#[derive(Clone, Copy, Debug)]
enum InterpBounds {
    Scalar(f32, f32),
    Vec2(Vec2, Vec2),
    Clr(Color, Color),
}

impl InterpBounds {
    fn unwrap_f32(self) -> (f32, f32) {
        if let Self::Scalar(s, e) = self {
            (s, e)
        } else {
            panic!("Try to unwrap as f32 but failed; it was {:?}", self);
        }
    }

    fn unwrap_vec2(self) -> (Vector2, Vector2) {
        if let Self::Vec2(s, e) = self {
            (s.vec2(), e.vec2())
        } else {
            panic!("Try to unwrap as vec2 but failed; it was {:?}", self);
        }
    }

    fn unwrap_clr(self) -> (Color, Color) {
        if let Self::Clr(s, e) = self {
            (s, e)
        } else {
            panic!("Try to unwrap as color but failed; it was {:?}", self);
        }
    }
}

trait Interp {
    fn interp(self, start: Self, end: Self, t: f32, duration: f32) -> Self
    where
        Self: Sized;
}

impl Interp for f32 {
    fn interp(self, start: Self, end: Self, t: f32, duration: f32) -> Self {
        let factor = rate_func(t, duration);
        start + factor * (end - start)
    }
}

impl Interp for Vector2 {
    fn interp(self, start: Self, end: Self, t: f32, duration: f32) -> Self {
        let factor = rate_func(t, duration);

        Self::add(start, Self::scale(Self::subtract(end, start), factor))
    }
}

impl Interp for Color {
    fn interp(self, start: Self, end: Self, t: f32, duration: f32) -> Self {
        let factor = rate_func(t, duration);

        let inv_factor = 1.0f32 - factor;

        Self {
            r: (inv_factor * start.r as f32 + factor * end.r as f32) as u8,
            g: (inv_factor * start.g as f32 + factor * end.g as f32) as u8,
            b: (inv_factor * start.b as f32 + factor * end.b as f32) as u8,
            a: (inv_factor * start.a as f32 + factor * end.a as f32) as u8,
        }
    }
}
