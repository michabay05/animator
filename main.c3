import std::io;
import rl;
import interp;

struct Circle
{
    Vector2 position;
    float radius;
    Color color;
}

fn void main()
{
    rl::init_window(800, 600, "Hello, from c3");
    rl::set_target_fps(60);
    defer rl::close_window();

    Circle c = {
        .position = {100, 100},
        .radius = 30,
        .color = rl::RED
    };

    interp::Interp{Vector2} interp1 = {
        .start = c.position,
        .value = &c.position,
        .end = {700, 500},
        .t = 0.0,
        .duration = 2.0
    };

    while (!rl::window_should_close()) {
        float dt = rl::get_frame_time();
        interp1.step(dt);

        rl::begin_drawing();
        rl::clear_background(rl::get_color(0x181818FF));

        rl::draw_circle_v(c.position, c.radius, c.color);

        rl::draw_fps(10, 10);
        rl::end_drawing();
    }
}
