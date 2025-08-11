import std::io;

import animator;
import ffmpeg;
import rl;

fn void main(String[] args)
{
    if (args.len == 1) {
        String program_name = args[0];
        io::printfn("USAGE: %s <INPUT_SCRIPT>", program_name);
        return;
    }

    rl::set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT.code);
    rl::init_window(800, 600, "Hello, from c3");
    defer rl::close_window();

    String script_path = args[1];
    Video v;
    v.init(script_path)!!;
    defer v.deinit();

    rl::set_target_fps(v.cfg.fps);

    while (!rl::window_should_close()) {
        if (rl::is_key_pressed(KeyboardKey.SPACE.code)) {
            v.paused = !v.paused;
        }
        if (rl::is_key_pressed(KeyboardKey.R.code) && v.can_render()) {
            v.duration = 0.0;
            v.rendering = true;
            v.paused = false;
            v.rtex = rl::load_render_texture(v.cfg.width, v.cfg.height);
            rl::set_trace_log_level(TraceLogLevel.WARNING);
            v.ffmpeg = ffmpeg::start_rendering_video(
                v.cfg.output_path, v.cfg.width, v.cfg.height, v.cfg.fps
            );
        }

        float dt = rl::get_frame_time();
        v.step(dt);

        rl::begin_drawing();
            if (v.ffmpeg) {
                rl::begin_texture_mode(v.rtex);
                    v.render();
                rl::end_texture_mode();

                rl::clear_background(rl::get_color(0x181818FF));
                rl::draw_text("rendering", 100, 100, 40, rl::RED);

                Image img = rl::load_image_from_texture(v.rtex.texture);
                defer rl::unload_image(img);
                if (!ffmpeg::send_frame_flipped(v.ffmpeg, img.data, img.width, img.height)) {
                    ffmpeg::end_rendering(v.ffmpeg, true);
                    io::printn("ERROR: Failed to send frame to ffmpeg");
                }

                if (v.duration >= v.max_duration) {
                    // TODO: handle boolean value here
                    ffmpeg::end_rendering(v.ffmpeg, false);
                    rl::set_trace_log_level(TraceLogLevel.INFO);
                    v.ffmpeg = null;
                    break;
                } else {
                    v.duration += dt;
                }
            } else {
                v.render();
            }
        rl::end_drawing();
    }
}
