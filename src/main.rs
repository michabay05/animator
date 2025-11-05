mod engine;
mod rl;

use rl::{Color, ConfigFlags, Image, Vector2, RenderTexture, TraceLogLevel};
use engine::{Context, Ffmpeg};

fn main() {
    rl::set_config_flags(&[ConfigFlags::Msaa4xHint, ConfigFlags::WindowResizable]);
    rl::set_trace_log_level(TraceLogLevel::WARNING);
    let factor = 300;
    let scr_sz = [4 * factor, 3 * factor];
    rl::init_window(scr_sz[0], scr_sz[1], "anim");
    let fps = 30;
    rl::set_target_fps(fps);

    let render_mode = false;
    let render_width = scr_sz[0];
    let render_height = scr_sz[1];
    let mut ffmpeg = Ffmpeg::new("out.mp4", render_width, render_height, fps);
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

            ffmpeg.start();

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
