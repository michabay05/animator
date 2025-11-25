#![allow(warnings)]

use std::f32::consts::PI;
use std::ffi::c_void;
use std::io::Write;
use std::os::fd::AsRawFd;
use std::process::{Child, Command, ExitStatus, Stdio};
use std::str::FromStr;

use crate::rl::{self, Color, Font, Image, KeyboardKey, Rectangle, Vector2};

#[derive(Debug)]
enum ObjKind {
    Text(TextObj),
    Rect(RectObj),
}

#[derive(Clone, Debug)]
pub struct Task<T> {
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
struct TaskProgress {
    t: f32,
    duration: f32,
}

impl TaskProgress {
    fn completed(&self) -> bool {
        return self.t >= self.duration;
    }
}

#[derive(Debug)]
pub enum TaskKind {
    MoveTo(usize, Task<Vector2>),
    Fade(usize, Task<Color>),
    Wait(TaskProgress),
    Scale(usize, Task<Vector2>),
}

impl TaskKind {
    fn completed(&self) -> bool {
        match self {
            TaskKind::Fade(_, ct) => ct.completed(),
            TaskKind::MoveTo(_, vt) => vt.completed(),
            TaskKind::Wait(tp) => tp.completed(),
            TaskKind::Scale(_, vt) => vt.completed(),
        }
    }
}

#[derive(Debug)]
pub struct Context {
    objs: Vec<ObjKind>,
    pub tasks: Vec<TaskKind>,
    task_idx: usize,
    pub paused: bool,
    pub completed: bool,
    background: Color,
}

impl Context {
    pub fn new() -> Self {
        Self {
            objs: vec![],
            tasks: vec![],
            task_idx: 0,
            paused: true,
            completed: false,
            background: Color::BLACK,
        }
    }

    pub fn update(&mut self, dt: f32) {
        if rl::is_window_resized() {
            for obj in &mut self.objs {
                match obj {
                    ObjKind::Text(t) => {
                        t.resize();
                    }
                    _ => {}
                }
            }
        }

        if rl::is_key_pressed(KeyboardKey::Space) {
            self.paused = !self.paused;
        }
        if rl::is_key_pressed(KeyboardKey::R) {
            unimplemented!();
        }

        if self.completed || self.paused || self.task_idx >= self.tasks.len() {
            return;
        }

        let mut task = &mut self.tasks[self.task_idx];
        if task.completed() {
            self.task_idx += 1;
            if self.task_idx >= self.tasks.len() {
                self.completed = true;
                return;
            }
            task = &mut self.tasks[self.task_idx];
        }

        match task {
            TaskKind::Fade(obj_id, ct) => match self.objs[*obj_id] {
                ObjKind::Text(ref mut t) => {
                    t.color.interp(dt, ct);
                }
                _ => unimplemented!(),
            },
            TaskKind::MoveTo(obj_id, vt) => match self.objs[*obj_id] {
                ObjKind::Text(ref mut t) => {
                    t.position.interp(dt, vt);
                }
                ObjKind::Rect(ref mut r) => {
                    r.position.interp(dt, vt);
                }
            },
            TaskKind::Wait(wt) => {
                wt.t += dt;
            }
            TaskKind::Scale(obj_id, vt) => match self.objs[*obj_id] {
                ObjKind::Rect(ref mut r) => {
                    r.size.interp(dt, vt);
                }
                _ => unimplemented!(),
            },
        }
    }

    pub fn render(&self) {
        rl::clear_background(self.background);

        for obj in &self.objs {
            match obj {
                ObjKind::Text(t) => t.render(),
                ObjKind::Rect(r) => r.render(),
            }
        }
    }

    pub fn text(&mut self, text: &str) -> TextObj {
        let t_obj = TextObj::new(
            text,
            "fonts/LibertinusSerif-Regular.ttf",
            5.0 / 60.0,
            Vector2::new(0.5, 0.5),
            Color::BLUE,
            self.objs.len(),
        );
        self.objs.push(ObjKind::Text(t_obj.clone()));
        t_obj
    }

    pub fn rect(&mut self) -> RectObj {
        let rect = RectObj::new(
            Vector2::new(0.5, 0.5),
            Vector2::new(0.25, 0.25),
            Color::WHITE,
            self.objs.len(),
        );
        self.objs.push(ObjKind::Rect(rect.clone()));
        rect
    }

    pub fn wait(&mut self, duration: f32) {
        self.tasks
            .push(TaskKind::Wait(TaskProgress { t: 0.0, duration }));
    }
}

#[derive(Clone, Debug)]
pub struct TextObj {
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
        id: usize,
    ) -> Self {
        let font = Font::from_path(font_path, font_size_ratio);
        let spacing = 0.0;
        let dimensions = rl::measure_text_ex(font.clone(), text, font.base_size as f32, spacing);
        Self {
            text: text.to_string(),
            font_path: font_path.to_string(),
            font_size_ratio,
            font,
            spacing,
            position,
            color,
            id,
            dimensions,
        }
    }

    fn resize(&mut self) {
        self.font.reload(&self.font_path, self.font_size_ratio);
        self.dimensions = rl::measure_text_ex(
            self.font.clone(),
            &self.text,
            self.font.base_size as f32,
            self.spacing,
        );
    }

    fn render(&self) {
        let screen_dim = Vector2::new(
            rl::get_screen_width() as f32,
            rl::get_screen_height() as f32,
        );
        let mut pos = Vector2::multiply(self.position, screen_dim);
        pos = Vector2::subtract(pos, Vector2::scale(self.dimensions, 0.5));

        rl::draw_text_ex(
            self.font.clone(),
            &self.text,
            pos,
            self.font.base_size as f32,
            0.0,
            self.color,
        );
    }

    pub fn move_to(&mut self, target: Vector2, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(self.position, target, duration);
        self.position = target;
        TaskKind::MoveTo(self.id, task)
    }

    pub fn fade(&mut self, target: Color, duration: f32) -> TaskKind {
        let task = Task::<Color>::new(self.color, target, duration);
        self.color = target;
        TaskKind::Fade(self.id, task)
    }
}

#[derive(Clone, Debug)]
pub struct RectObj {
    position: Vector2,
    size: Vector2,
    color: Color,
    id: usize,
}

impl RectObj {
    fn new(position: Vector2, size: Vector2, color: Color, id: usize) -> Self {
        Self {
            position,
            size,
            color,
            id,
        }
    }

    fn render(&self) {
        let screen_dim = Vector2::new(
            rl::get_screen_width() as f32,
            rl::get_screen_height() as f32,
        );
        let size = Vector2::multiply(self.size, screen_dim);
        let pos = Vector2::subtract(
            Vector2::multiply(self.position, screen_dim),
            Vector2::scale(size, 0.5),
        );
        let rec = Rectangle {
            x: pos.x,
            y: pos.y,
            width: size.x,
            height: size.y,
        };
        rl::draw_rectangle_pro(rec, Vector2::zero(), 0.0, self.color);
    }

    pub fn move_to(&mut self, target: Vector2, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(self.position, target, duration);
        self.position = target;
        TaskKind::MoveTo(self.id, task)
    }

    pub fn grow(&mut self, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(Vector2::zero(), self.size, duration);
        TaskKind::Scale(self.id, task)
    }
}

trait Interp {
    fn interp(&mut self, dt: f32, info: &mut Task<Self>)
    where
        Self: Sized;
}

impl Interp for Vector2 {
    fn interp(&mut self, dt: f32, task: &mut Task<Self>) {
        let factor = rate_func(task.t, task.duration);
        task.t += dt;

        *self = Self::add(
            task.start,
            Self::scale(Self::subtract(task.end, task.start), factor),
        );
    }
}

impl Interp for Color {
    fn interp(&mut self, dt: f32, task: &mut Task<Self>) {
        let factor = rate_func(task.t, task.duration);
        let inv_factor = 1.0f32 - factor;
        task.t += dt;

        let start = task.start;
        let end = task.end;
        *self = Self {
            r: (inv_factor * start.r as f32 + factor * end.r as f32) as u8,
            g: (inv_factor * start.g as f32 + factor * end.g as f32) as u8,
            b: (inv_factor * start.b as f32 + factor * end.b as f32) as u8,
            a: (inv_factor * start.a as f32 + factor * end.a as f32) as u8,
        };
    }
}

fn rate_func(t: f32, duration: f32) -> f32 {
    if t >= duration {
        1.0
    } else {
        // >> Linear
        // t / duration

        // >> Sinusoidal
        -0.5 * ((PI / duration) * t).cos() + 0.5
    }
}

pub struct Ffmpeg {
    child: Option<Child>,
    width: i32,
    height: i32,
    fps: i32,
    output_path: String,
    is_rendering: bool,
}

impl Ffmpeg {
    // const FFMPEG_BINARY: &str = "C:\\ProgramData\\chocolatey\\bin\\ffmpeg.exe";
    const FFMPEG_BINARY: &str = "ffmpeg";

    pub fn new(output_path: &str, width: i32, height: i32, fps: i32) -> Self {
        Self {
            child: None,
            width,
            height,
            fps,
            output_path: output_path.to_string(),
            is_rendering: false
        }
    }

    #[rustfmt::skip]
    pub fn start(&mut self) {
        if self.is_rendering {
            eprintln!("[WARN] Another ffmpeg process is already rendering.");
            return;
        }

        let resolution = format!("{}x{}", self.width, self.height);
        let framerate = format!("{}", self.fps);

        self.child = Some(Command::new(Self::FFMPEG_BINARY)
            .args([
                "-loglevel", "verbose",
                "-y",

                "-f", "rawvideo",
                "-pix_fmt", "rgba",
                "-s", &resolution,
                "-r", &framerate,
                "-i", "-",

                "-c:v", "libx264",
                "-vb", "2500k",
                // "-c:a", "aac",
                // "-ab", "200k",
                "-pix_fmt", "yuv420p",
                &self.output_path,
            ])
            .stdin(Stdio::piped())
            .spawn()
            .expect("Failed to execute command"));

        self.is_rendering = true;
    }

    pub fn send_frame(&mut self, image: &Image) -> Result<(), String> {
        if self.child.is_none() {
            return Err(format!("There is no child process"));
        }

        let child = self.child.as_mut().unwrap();
        assert!(false, "Bad code below; stay away...");

        if let Some(ref c_in) = child.stdin {
            let raw_fd = c_in.as_raw_fd();
            let mut count = 0;
            for y in (1..=image.height).rev() {
                // Source: https://github.com/tsoding/panim/blob/main/panim/ffmpeg_linux.c

                // stride: sizeof(uint32_t)*width
                let u32_size = (u32::BITS / 8) as usize;
                let stride = u32_size * image.width as usize;

                // let buf = (image.data as *const u32).wrapping_add(((y - 1)*image.width) as usize);
                // let buf = (image.data as *const u32).wrapping_add((y * image.width) as usize);
                let buf =
                    unsafe { (image.data as *const u32).add(((y - 1) * image.width) as usize) };
                if true {
                    eprintln!("{} successful rows have been written", count);
                    self.is_rendering = false;
                    return Err(format!("FFMPEG: failed to write frame into ffmpeg's stdin"));
                } else {
                    count += 1;
                }
            }
            Ok(())
        } else {
            Err(format!("FFMPEG: failed to access ffmpeg's stdin handle"))
        }
    }

    pub fn stop(&mut self) -> Option<ExitStatus> {
        if self.child.is_none() {
            return None;
        }

        let child = self.child.as_mut().unwrap();
        if let Some(ref mut c_in) = child.stdin {
            let _ = c_in.flush();
        }
        child.stdin.take();
        let result = child.wait();
        self.child = None;
        self.is_rendering = false;

        result.ok()
    }
}
