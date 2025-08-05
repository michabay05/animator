import std::io;
import std::collections::list;
import animator;
import rl;
import ffmpeg;


fn void main()
{
    rl::set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT.code);
    rl::init_window(800, 600, "Hello, from c3");
    rl::set_target_fps(60);
    defer rl::close_window();

    Circle c = {
        .position = {100, 100},
        .radius = 30,
        .color = rl::RED
    };

    Rectangle r = {
        .position = {300, 100},
        .size = {150, 75},
        .color = rl::BEIGE
    };

    V2Interp v = {
        .start = r.size,
        .value = &r.size,
        .end = {300, 200},
        .t = 0.0,
        .duration = 2.0
    };

    FltInterp f = {
        .start = c.radius,
        .value = &c.radius,
        .end = 60,
        .t = 0.0,
        .duration = 1.0
    };

    List{Interpolable} interps;
    {
        interps.push(&v);
        interps.push(&f);
        interps.push(&&((V2Interp){
                    .start = c.position,
                    .value = &c.position,
                    .end = {c.position.x, 500},
                    .t = 0.0,
                    .duration = 2.0
                    }));
    }

    float duration = 0.0;
    while (!rl::window_should_close()) {
        float dt = rl::get_frame_time();
        foreach (&interp : interps) {
            interp.step(dt);
        }
        duration += dt;

        rl::begin_drawing();
            rl::clear_background(rl::get_color(0x181818FF));
            rl::draw_circle_v(c.position, c.radius, c.color);
            rl::draw_rectangle_v(r.position, r.size, r.color);
        rl::end_drawing();
    }
}
