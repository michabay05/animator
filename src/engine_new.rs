use crate::acode::{self, Instruction, ObjInfo};
use crate::rl::{self, Color, ConfigFlags, TraceLogLevel, Vector2};

#[derive(Debug)]
struct RectInfo {
    position: Vector2,
    size: Vector2,
    color: Color,
}

impl Default for RectInfo {
    fn default() -> Self {
        Self {
            position: Vector2::zero(),
            size: Vector2::one(),
            color: Color::WHITE
        }
    }
}

#[derive(Debug)]
struct TextInfo {
    text: String,
    position: Vector2,
    color: Color
}

impl TextInfo {
    fn default(s: &str) -> Self {
        Self {
            text: s.to_string(),
            position: Vector2::zero(),
            color: Color::WHITE
        }
    }
}

#[derive(Debug)]
enum ObjKind {
    Rect(RectInfo),
    Text(TextInfo)
}

struct Obj {
    kind: ObjKind,
    enabled: bool
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
            enabled: false
        }
    }
}

struct Context {
    insts: Vec<Instruction>,
    objs: Vec<Obj>,
}

impl Context {
    fn from_path(filepath: &str) -> Self {
        let mut this = Self {
            insts: acode::parse(filepath),
            objs: vec![]
        };
        this.setup();
        this
    }

    fn setup(&mut self) {
        for inst in &self.insts {
            match inst {
                Instruction::Create(info) => match info {
                    ObjInfo::Rect => self.objs.push(Obj::rect()),
                    ObjInfo::Text(s) => self.objs.push(Obj::text(s)),
                }
                // Other instructions are ignored for now but will be ran later.
                _ => {}
            }
        }

        dbg!(&self.objs);
    }

    fn update(&mut self, _dt: f32) {}

    fn render(&mut self) {
        rl::clear_background(Color::BLACK);
    }
}

pub fn engine_expr() {
    rl::set_config_flags(&[ConfigFlags::Msaa4xHint, ConfigFlags::WindowResizable]);
    rl::set_trace_log_level(TraceLogLevel::WARNING);
    let factor = 300;
    let scr_sz = [4 * factor, 3 * factor];
    rl::init_window(scr_sz[0], scr_sz[1], "anim");

    let mut ctx = Context::from_path("acodes/simple.ac");

    while !rl::window_should_close() {
        ctx.update(rl::get_frame_time());

        rl::begin_drawing();
        ctx.render();
        rl::end_drawing();
    }

    rl::close_window();
}
