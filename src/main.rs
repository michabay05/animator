mod rl;

use std::f32::consts::PI;
use std::ffi::c_void;
use std::io::Write;
use std::os::fd::AsRawFd;
use std::process::{Child, Command, ExitStatus, Stdio};

use rl::{
    Color, ConfigFlags, Font, Image, KeyboardKey, Rectangle, RenderTexture, TraceLogLevel, Vector2,
};

#[derive(Debug)]
enum ObjKind {
    Text(TextObj),
    Rect(RectObj),
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
enum TaskKind {
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
struct Context {
    objs: Vec<ObjKind>,
    tasks: Vec<TaskKind>,
    task_idx: usize,
    paused: bool,
    completed: bool,
    background: Color,
}

impl Context {
    fn new() -> Self {
        Self {
            objs: vec![],
            tasks: vec![],
            task_idx: 0,
            paused: true,
            completed: false,
            background: Color::BLACK,
        }
    }

    fn update(&mut self, dt: f32) {
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

    fn render(&self) {
        rl::clear_background(self.background);

        for obj in &self.objs {
            match obj {
                ObjKind::Text(t) => t.render(),
                ObjKind::Rect(r) => r.render(),
            }
        }
    }

    fn text(&mut self, text: &str) -> TextObj {
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

    fn rect(&mut self) -> RectObj {
        let rect = RectObj::new(
            Vector2::new(0.5, 0.5),
            Vector2::new(0.25, 0.25),
            Color::WHITE,
            self.objs.len(),
        );
        self.objs.push(ObjKind::Rect(rect.clone()));
        rect
    }

    fn wait(&mut self, duration: f32) {
        self.tasks
            .push(TaskKind::Wait(TaskProgress { t: 0.0, duration }));
    }
}

fn main() {
    rl::set_config_flags(&[ConfigFlags::Msaa4xHint, ConfigFlags::WindowResizable]);
    rl::set_trace_log_level(TraceLogLevel::WARNING);
    let factor = 300;
    let scr_sz = [4 * factor, 3 * factor];
    rl::init_window(scr_sz[0], scr_sz[1], "anim");
    let fps = 30;
    rl::set_target_fps(fps);

    let render_mode = true;
    let render_width = scr_sz[0];
    let render_height = scr_sz[1];
    let mut ffmpeg = Ffmpeg::start("out.mp4", render_width, render_height, fps);
    let rtex = RenderTexture::new(rl::get_screen_width(), rl::get_screen_height());

    let mut ctx = Context::new();
    let mut rect = ctx.rect();
    let mut hdr = ctx.text("SAT Geometry Problem");

    ctx.tasks.push(rect.grow(1.5));
    ctx.wait(1.0);
    ctx.tasks.push(hdr.fade(Color::RED, 1.5));
    ctx.wait(1.0);
    ctx.tasks.push(rect.move_to(Vector2::new(0.3, 0.8), 2.0));

    while !rl::window_should_close() {
        ctx.update(rl::get_frame_time());

        rl::begin_drawing();
        if render_mode {
            if ctx.paused {
                ctx.paused = false;
            }
            rl::begin_texture_mode(rtex.clone());
            ctx.render();
            rl::end_texture_mode();

            let image = Image::from_texture(rtex.texture.clone());
            ffmpeg.send_frame(&image).unwrap();
            image.unload();

            if ctx.completed {
                eprintln!("[DEBUG] Context completed...");
                break;
            }
        } else {
            ctx.render();
            rl::draw_fps(15, 15);
            if ctx.completed {
                rl::draw_text("Completed", 15, 45, 20, Color::WHITE);
            }
        }
        rl::end_drawing();
    }

    dbg!(ffmpeg.stop());
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

#[derive(Clone, Debug)]
struct RectObj {
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

    fn move_to(&mut self, target: Vector2, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(self.position, target, duration);
        self.position = target;
        TaskKind::MoveTo(self.id, task)
    }

    fn grow(&mut self, duration: f32) -> TaskKind {
        let task = Task::<Vector2>::new(Vector2::zero(), self.size, duration);
        TaskKind::Scale(self.id, task)
    }
}

struct Ffmpeg {
    child: Child,
}

impl Ffmpeg {
    // const FFMPEG_BINARY: &str = "C:\\ProgramData\\chocolatey\\bin\\ffmpeg.exe";
    const FFMPEG_BINARY: &str = "ffmpeg";

    #[rustfmt::skip]
    fn start(output_path: &str, width: i32, height: i32, fps: i32) -> Self {
        let resolution = format!("{}x{}", width, height);
        let framerate = format!("{}", fps);

        let child = Command::new(Self::FFMPEG_BINARY)
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
                output_path,
            ])
            .stdin(Stdio::piped())
            .spawn()
            .expect("Failed to execute command");

        Self { child }
    }

    fn send_frame(&mut self, image: &Image) -> Result<(), String> {
        if let Some(ref c_in) = self.child.stdin {
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
                if unsafe { libc::write(raw_fd, buf as *const c_void, stride) } < 0 {
                    eprintln!("{} successful rows have been written", count);
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

    fn stop(&mut self) -> ExitStatus {
        // self.child.wait().unwrap().code()
        if let Some(ref mut c_in) = self.child.stdin {
            let _ = c_in.flush();
        }
        self.child.stdin.take();
        self.child.wait().unwrap()
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

// const TYPST_PATH: &str = "C:\\Dev\\thirdparty\\typst\\typst.exe";
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
