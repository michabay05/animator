#![allow(warnings)]

mod rl;

use std::f32::consts::PI;
use std::fmt::Write;
use std::io;
use std::process::{Command, Stdio};
use std::time::SystemTime;

use rl::{
    Color, ConfigFlags, Font, Image, KeyboardKey, Texture, TextureFilter, TraceLogLevel, Vector2,
};

#[derive(Debug)]
enum ObjKind {
    Text(TextObj),
}

#[derive(Clone, Debug)]
struct Task<T> {
    start: T,
    end: T,
    t: f32,
    duration: f32,
}

impl<T> Task<T> {
    fn new(start: T, end: T, duration: f32) -> Self {
        Self {
            start,
            end,
            t: 0.0,
            duration,
        }
    }

    fn completed(&self) -> bool {
        return self.t >= self.duration;
    }
}

#[derive(Clone, Debug)]
struct WaitTask {
    t: f32,
    duration: f32
}

impl WaitTask {
    fn completed(&self) -> bool {
        return self.t >= self.duration
    }
}

#[derive(Debug)]
enum TaskKind {
    MoveTo(usize, Task<Vector2>),
    Fade(usize, Task<Color>),
    Wait(WaitTask)
}

impl TaskKind {
    fn completed(&self) -> bool {
        match self {
            TaskKind::Fade(_, ct) => {
                ct.completed()
            },
            TaskKind::MoveTo(_, vt) => {
                vt.completed()
            }
            TaskKind::Wait(wt) => {
                wt.completed()
            }
        }
    }
}

#[derive(Debug)]
struct Context {
    objs: Vec<ObjKind>,
    tasks: Vec<TaskKind>,
    task_idx: usize,
    paused: bool,
    background: Color,
}

impl Context {
    fn new() -> Self {
        Self {
            objs: vec![],
            tasks: vec![],
            task_idx: 0,
            paused: true,
            background: Color::BEIGE_LT,
        }
    }

    fn update(&mut self, dt: f32) {
        if rl::is_window_resized() {
            for obj in &mut self.objs {
                match obj {
                    ObjKind::Text(t) => {
                        t.resize();
                    }
                }
            }
        }

        if rl::is_key_pressed(KeyboardKey::Space) {
            self.paused = !self.paused;
        }
        if rl::is_key_pressed(KeyboardKey::R) {
            unimplemented!();
        }

        if self.paused || self.task_idx >= self.tasks.len() {
            return;
        }

        let mut task = &mut self.tasks[self.task_idx];
        if task.completed() {
            self.task_idx += 1;
            if self.task_idx >= self.tasks.len() {
                return;
            }
            task = &mut self.tasks[self.task_idx];
        }

        match task {
            TaskKind::Fade(obj_id, ct) => {
                match self.objs[*obj_id] {
                    ObjKind::Text(ref mut t) => {
                        t.color.interp(dt, ct);
                    }
                }
            },
            TaskKind::MoveTo(obj_id, vt) => {
                match self.objs[*obj_id] {
                    ObjKind::Text(ref mut t) => {
                        t.position.interp(dt, vt);
                    }
                }
            }
            TaskKind::Wait(wt) => {
                wt.t += dt;
            }
        }
    }

    fn render(&self) {
        rl::clear_background(self.background);
        
        for obj in &self.objs {
            match obj {
                ObjKind::Text(t) => t.render()
            }
        }
    }

    fn text(&mut self,
        text: &str,
        font_path: &str,
        font_size_ratio: f32,
        position: Vector2,
        color: Color,
    ) -> TextObj {
        let t_obj = TextObj::new(
            text, font_path, font_size_ratio, position, color, self.objs.len()
        );
        self.objs.push(ObjKind::Text(t_obj.clone()));
        t_obj
    }

    fn wait(&mut self, duration: f32)  {
        self.tasks.push(
            TaskKind::Wait(WaitTask { t: 0.0, duration })
        );
    }
}

fn main() {
    rl::set_config_flags(&[ConfigFlags::Msaa4xHint, ConfigFlags::WindowResizable]);
    // rl::set_trace_log_level(TraceLogLevel::WARNING);
    rl::init_window(800, 600, "anim");
    rl::set_target_fps(45);

    let mut ctx = Context::new();
    let mut hdr = ctx.text(
        "SAT Geometry Problem",
        "fonts/LibertinusSerif-Bold.ttf",
        5.0 / 60.0,
        Vector2::new(0.5, 0.5),
        Color::from_hex(0x3f6d7fff),
    );
    ctx.tasks.push(hdr.fade(Color::RED, 1.5));
    ctx.wait(1.5);
    ctx.tasks.push(hdr.move_to(Vector2::new(0.3, 0.3), 2.0));
    ctx.tasks.push(hdr.fade(Color::BLUE, 1.0));

    /*
        - Create the object
        - Clone object and save to the ctx objects list
        - Using the original object's methods create tasks that will be appended to the task list
    */

    while !rl::window_should_close() {
        ctx.update(rl::get_frame_time());

        rl::begin_drawing();
        ctx.render();
        rl::draw_fps(15, 15);
        rl::end_drawing();
    }

    rl::close_window();
}

#[derive(Clone, Debug)]
struct TextObj {
    text: String,
    font_path: String,
    font_size_ratio: f32,
    font: Font,
    position: Vector2,
    color: Color,
    spacing: f32,
    id: usize,
    dimensions: Vector2,
}

impl TextObj {
    fn new(
        text: &str,
        font_path: &str,
        font_size_ratio: f32,
        position: Vector2,
        color: Color,
        id: usize
    ) -> Self {
        let mut font = Font::from_path(font_path, font_size_ratio);
        let spacing = 0.0;
        let dimensions = rl::measure_text_ex(
            font.clone(), text,
            font.base_size as f32,
            spacing
        );
        Self {
            text: text.to_string(),
            font_path: font_path.to_string(),
            font_size_ratio,
            font,
            spacing,
            position,
            color,
            id,
            dimensions
        }
    }

    fn resize(&mut self) {
        self.font.reload(&self.font_path, self.font_size_ratio);
        self.dimensions = rl::measure_text_ex(
            self.font.clone(), &self.text,
            self.font.base_size as f32,
            self.spacing
        );
    }

    fn render(&self) {
        let screen_dim = Vector2::new(
            rl::get_screen_width() as f32,
            rl::get_screen_height() as f32,
        );
        let mut pos = Vector2::multiply(self.position, screen_dim);
        pos = Vector2::subtract(
            pos, Vector2::scale(self.dimensions, 0.5)
        );

        rl::draw_text_ex(
            self.font.clone(),
            &self.text,
            pos,
            self.font.base_size as f32,
            0.0,
            self.color,
        );
    }

    fn move_to(&mut self, target: Vector2, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(self.position, target, duration);
        self.position = target;
        TaskKind::MoveTo(self.id, task)
    }

    fn fade(&mut self, target: Color, duration: f32) -> TaskKind {
        let task = Task::<Color>::new(self.color, target, duration);
        self.color = target;
        TaskKind::Fade(self.id, task)
    }
}

trait Interp {
    fn interp(&mut self, dt: f32, info: &mut Task<Self>)
    where
        Self: Sized;
}

impl Interp for Vector2 {
    fn interp(&mut self, dt: f32, task: &mut Task<Self>) {
        let factor = if task.t >= task.duration {
            1.0
        } else {
            // task.t / task.duration // Linear
            -0.5 * ((PI / task.duration) * task.t).cos() + 0.5 // Sinusoidal
        };
        task.t += dt;

        *self = Self::add(
            task.start,
            Self::scale(Self::subtract(task.end, task.start), factor),
        );
    }
}

impl Interp for Color {
    fn interp(&mut self, dt: f32, task: &mut Task<Self>) {
        let factor = if task.t >= task.duration {
            1.0
        } else {
            // task.t / task.duration // Linear
            -0.5 * ((PI / task.duration) * task.t).cos() + 0.5 // Sinusoidal
        };
        task.t += dt;

        let start = task.start;
        let end = task.end;
        *self = Self {
            r: ((1.0f32 - factor)*start.r as f32 + factor*end.r as f32) as u8,
            g: ((1.0f32 - factor)*start.g as f32 + factor*end.g as f32) as u8,
            b: ((1.0f32 - factor)*start.b as f32 + factor*end.b as f32) as u8,
            a: ((1.0f32 - factor)*start.a as f32 + factor*end.a as f32) as u8,
        };
    }
}

// const TYPST_PATH: &str = "C:\\Dev\\thirdparty\\typst\\typst.exe";
// const FFMPEG_PATH: &str = "C:\\ProgramData\\chocolatey\\bin\\ffmpeg.exe";
// fn gen_typst(source: &str, output_path: &str) -> io::Result<()> {
//     use std::io::{Read, Write};
//
//     let mut child = Command::new(TYPST_PATH)
//         .args(["compile", "-", output_path])
//         .stdin(Stdio::piped())
//         .spawn()
//         .expect("Failed to execute command");
//
//     if let Some(mut c_in) = child.stdin.take() {
//         writeln!(c_in, "{source}")?;
//         println!("[INFO] compiled typst");
//     } else {
//         eprintln!("[ERROR] could get the child's stdin");
//     }
//
//     Ok(())
// }
