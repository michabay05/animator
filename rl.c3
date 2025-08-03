alias Vector2 = float[<2>];
alias Color = char[<4>];
alias Colorf32 = float[<4>];

const Color RED = { 230, 41, 55, 255 };
const Color SKYBLUE = { 102, 191, 255, 255 };
const Color BEIGE = { 211, 176, 131, 255 };

/* ======== Window ======== */
extern fn int init_window(CInt width, CInt height, CChar *title) @extern("InitWindow");
extern fn int close_window() @extern("CloseWindow");
extern fn bool window_should_close() @extern("WindowShouldClose");

/* ======== Drawing ======== */
extern fn void begin_drawing() @extern("BeginDrawing");
extern fn void end_drawing() @extern("EndDrawing");
extern fn void clear_background(Color color) @extern("ClearBackground");
extern fn void draw_circle_v(Vector2 position, float radius, Color color) @extern("DrawCircleV");
extern fn void draw_fps(CInt posX, CInt posY) @extern("DrawFPS");

/* ======== Miscellaneous ======== */
extern fn float get_frame_time() @extern("GetFrameTime");
extern fn void set_target_fps(CInt fps) @extern("SetTargetFPS");
extern fn Color get_color(CUInt hexValue) @extern("GetColor");

