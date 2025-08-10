alias Vector2 = float[<2>];
alias Color = char[<4>];
alias Colorf32 = float[<4>];

const Color RED = { 230, 41, 55, 255 };
const Color SKYBLUE = { 102, 191, 255, 255 };
const Color BEIGE = { 211, 176, 131, 255 };
const Color WHITE = { 255, 255, 255, 255 };

enum ConfigFlags : int (int code) {
    FLAG_VSYNC_HINT         = 0x00000040,   // Set to try enabling V-Sync on GPU
    FLAG_FULLSCREEN_MODE    = 0x00000002,   // Set to run program in fullscreen
    FLAG_WINDOW_RESIZABLE   = 0x00000004,   // Set to allow resizable window
    FLAG_WINDOW_UNDECORATED = 0x00000008,   // Set to disable window decoration (frame and buttons)
    FLAG_WINDOW_HIDDEN      = 0x00000080,   // Set to hide window
    FLAG_WINDOW_MINIMIZED   = 0x00000200,   // Set to minimize window (iconify)
    FLAG_WINDOW_MAXIMIZED   = 0x00000400,   // Set to maximize window (expanded to monitor)
    FLAG_WINDOW_UNFOCUSED   = 0x00000800,   // Set to window non focused
    FLAG_WINDOW_TOPMOST     = 0x00001000,   // Set to window always on top
    FLAG_WINDOW_ALWAYS_RUN  = 0x00000100,   // Set to allow windows running while minimized
    FLAG_WINDOW_TRANSPARENT = 0x00000010,   // Set to allow transparent framebuffer
    FLAG_WINDOW_HIGHDPI     = 0x00002000,   // Set to support HighDPI
    FLAG_WINDOW_MOUSE_PASSTHROUGH = 0x00004000, // Set to support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
    FLAG_BORDERLESS_WINDOWED_MODE = 0x00008000, // Set to run program in borderless windowed mode
    FLAG_MSAA_4X_HINT       = 0x00000020,   // Set to try enabling MSAA 4X
    FLAG_INTERLACED_HINT    = 0x00010000    // Set to try enabling interlaced video format (for V3D)
}

enum TraceLogLevel
{
    ALL,      // Display all logs
    TRACE,    // Trace logging, intended for internal use only
    DEBUG,    // Debug logging, used for internal debugging, it should be disabled on release builds
    INFO,     // Info logging, used for program execution info
    WARNING,  // Warning logging, used on recoverable failures
    ERROR,    // Error logging, used on unrecoverable failures
    FATAL,    // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    NONE      // Disable logging
}

// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
enum KeyboardKey : int (int code)
{
    NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    APOSTROPHE      = 39,       // Key: '
    COMMA           = 44,       // Key: ,
    MINUS           = 45,       // Key: -
    PERIOD          = 46,       // Key: .
    SLASH           = 47,       // Key: /
    ZERO            = 48,       // Key: 0
    ONE             = 49,       // Key: 1
    TWO             = 50,       // Key: 2
    THREE           = 51,       // Key: 3
    FOUR            = 52,       // Key: 4
    FIVE            = 53,       // Key: 5
    SIX             = 54,       // Key: 6
    SEVEN           = 55,       // Key: 7
    EIGHT           = 56,       // Key: 8
    NINE            = 57,       // Key: 9
    SEMICOLON       = 59,       // Key: ;
    EQUAL           = 61,       // Key: =
    A               = 65,       // Key: A | a
    B               = 66,       // Key: B | b
    C               = 67,       // Key: C | c
    D               = 68,       // Key: D | d
    E               = 69,       // Key: E | e
    F               = 70,       // Key: F | f
    G               = 71,       // Key: G | g
    H               = 72,       // Key: H | h
    I               = 73,       // Key: I | i
    J               = 74,       // Key: J | j
    K               = 75,       // Key: K | k
    L               = 76,       // Key: L | l
    M               = 77,       // Key: M | m
    N               = 78,       // Key: N | n
    O               = 79,       // Key: O | o
    P               = 80,       // Key: P | p
    Q               = 81,       // Key: Q | q
    R               = 82,       // Key: R | r
    S               = 83,       // Key: S | s
    T               = 84,       // Key: T | t
    U               = 85,       // Key: U | u
    V               = 86,       // Key: V | v
    W               = 87,       // Key: W | w
    X               = 88,       // Key: X | x
    Y               = 89,       // Key: Y | y
    Z               = 90,       // Key: Z | z
    LEFT_BRACKET    = 91,       // Key: [
    BACKSLASH       = 92,       // Key: '\'
    RIGHT_BRACKET   = 93,       // Key: ]
    GRAVE           = 96,       // Key: `
    // Function keys
    SPACE           = 32,       // Key: Space
    ESCAPE          = 256,      // Key: Esc
    ENTER           = 257,      // Key: Enter
    TAB             = 258,      // Key: Tab
    BACKSPACE       = 259,      // Key: Backspace
    INSERT          = 260,      // Key: Ins
    DELETE          = 261,      // Key: Del
    RIGHT           = 262,      // Key: Cursor right
    LEFT            = 263,      // Key: Cursor left
    DOWN            = 264,      // Key: Cursor down
    UP              = 265,      // Key: Cursor up
    PAGE_UP         = 266,      // Key: Page up
    PAGE_DOWN       = 267,      // Key: Page down
    HOME            = 268,      // Key: Home
    END             = 269,      // Key: End
    CAPS_LOCK       = 280,      // Key: Caps lock
    SCROLL_LOCK     = 281,      // Key: Scroll down
    NUM_LOCK        = 282,      // Key: Num lock
    PRINT_SCREEN    = 283,      // Key: Print screen
    PAUSE           = 284,      // Key: Pause
    F1              = 290,      // Key: F1
    F2              = 291,      // Key: F2
    F3              = 292,      // Key: F3
    F4              = 293,      // Key: F4
    F5              = 294,      // Key: F5
    F6              = 295,      // Key: F6
    F7              = 296,      // Key: F7
    F8              = 297,      // Key: F8
    F9              = 298,      // Key: F9
    F10             = 299,      // Key: F10
    F11             = 300,      // Key: F11
    F12             = 301,      // Key: F12
    LEFT_SHIFT      = 340,      // Key: Shift left
    LEFT_CONTROL    = 341,      // Key: Control left
    LEFT_ALT        = 342,      // Key: Alt left
    LEFT_SUPER      = 343,      // Key: Super left
    RIGHT_SHIFT     = 344,      // Key: Shift right
    RIGHT_CONTROL   = 345,      // Key: Control right
    RIGHT_ALT       = 346,      // Key: Alt right
    RIGHT_SUPER     = 347,      // Key: Super right
    KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    KP_0            = 320,      // Key: Keypad 0
    KP_1            = 321,      // Key: Keypad 1
    KP_2            = 322,      // Key: Keypad 2
    KP_3            = 323,      // Key: Keypad 3
    KP_4            = 324,      // Key: Keypad 4
    KP_5            = 325,      // Key: Keypad 5
    KP_6            = 326,      // Key: Keypad 6
    KP_7            = 327,      // Key: Keypad 7
    KP_8            = 328,      // Key: Keypad 8
    KP_9            = 329,      // Key: Keypad 9
    KP_DECIMAL      = 330,      // Key: Keypad .
    KP_DIVIDE       = 331,      // Key: Keypad /
    KP_MULTIPLY     = 332,      // Key: Keypad *
    KP_SUBTRACT     = 333,      // Key: Keypad -
    KP_ADD          = 334,      // Key: Keypad +
    KP_ENTER        = 335,      // Key: Keypad Enter
    KP_EQUAL        = 336,      // Key: Keypad =
    // Android key buttons
    BACK            = 4,        // Key: Android back button
    MENU            = 5,        // Key: Android menu button
    VOLUME_UP       = 24,       // Key: Android volume up button
    VOLUME_DOWN     = 25        // Key: Android volume down button
}

// Texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
enum TextureFilter {
    POINT,                   // No filter, just pixel approximation
    BILINEAR,                // Linear filtering
    TRILINEAR,               // Trilinear filtering (linear with mipmaps)
    ANISOTROPIC_4X,          // Anisotropic filtering 4x
    ANISOTROPIC_8X,          // Anisotropic filtering 8x
    ANISOTROPIC_16X,         // Anisotropic filtering 16x
}

// Rectangle, 4 components
struct Rectangle {
    float x;                // Rectangle top-left corner position x
    float y;                // Rectangle top-left corner position y
    float width;            // Rectangle width
    float height;           // Rectangle height
}

// Image, pixel data stored in CPU memory (RAM)
struct Image {
    void *data;             // Image raw data
    int width;              // Image base width
    int height;             // Image base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
}

// Texture, tex data stored in GPU memory (VRAM)
struct Texture {
    uint id;      // OpenGL texture id
    int width;    // Texture base width
    int height;   // Texture base height
    int mipmaps;  // Mipmap levels, 1 by default
    int format;   // Data format (PixelFormat type)
}
alias Texture2D = Texture;

// RenderTexture, fbo for texture rendering
struct RenderTexture {
    uint id;          // OpenGL framebuffer object id
    Texture texture;  // Color buffer attachment texture
    Texture depth;    // Depth buffer attachment texture
}
alias RenderTexture2D = RenderTexture;

// GlyphInfo, font characters glyphs info
struct GlyphInfo {
    int value;              // Character value (Unicode)
    int offsetX;            // Character offset X when drawing
    int offsetY;            // Character offset Y when drawing
    int advanceX;           // Character advance position X
    Image image;            // Character image data
}

// Font, font texture and GlyphInfo array data
struct Font {
    int baseSize;           // Base size (default chars height)
    int glyphCount;         // Number of glyph characters
    int glyphPadding;       // Padding around the glyph characters
    Texture2D texture;      // Texture atlas containing the glyphs
    Rectangle *recs;        // Rectangles in texture for the glyphs
    GlyphInfo *glyphs;      // Glyphs info data
}

/* ======== Window ======== */
extern fn int init_window(CInt width, CInt height, char *title) @extern("InitWindow");
extern fn int close_window() @extern("CloseWindow");
extern fn bool window_should_close() @extern("WindowShouldClose");
extern fn void set_config_flags(uint flags) @extern("SetConfigFlags");

/* ======== Drawing ======== */
extern fn void begin_drawing() @extern("BeginDrawing");
extern fn void end_drawing() @extern("EndDrawing");
extern fn void clear_background(Color color) @extern("ClearBackground");
extern fn void draw_circle_v(Vector2 position, float radius, Color color) @extern("DrawCircleV");
extern fn void draw_rectangle_v(Vector2 position, Vector2 size, Color color) @extern("DrawRectangleV");
extern fn void draw_fps(CInt posX, CInt posY) @extern("DrawFPS");
extern fn void begin_texture_mode(RenderTexture2D target) @extern("BeginTextureMode");
extern fn void end_texture_mode() @extern("EndTextureMode");
extern fn void draw_text(char *text, CInt posX, CInt posY, CInt fontSize, Color color) @extern("DrawText");
extern fn void draw_text_ex(
    Font font, char *text, Vector2 position, float fontSize,
    float spacing, Color tint
) @extern("DrawTextEx");
extern fn void draw_line_ex(Vector2 startPos, Vector2 endPos, float thick, Color color) @extern("DrawLineEx");

/* ======== Image & Texture ======== */
extern fn Image load_image_from_texture(Texture2D rtex) @extern("LoadImageFromTexture");
extern fn void unload_image(Image image) @extern("UnloadImage");
extern fn bool export_image(Image image, char *fileName) @extern("ExportImage");

extern fn RenderTexture2D load_render_texture(CInt width, CInt height) @extern("LoadRenderTexture");
extern fn void unload_render_texture(RenderTexture2D target) @extern("UnloadRenderTexture");
extern fn void set_texture_filter(Texture2D texture, int filter) @extern("SetTextureFilter");

/* ======== Font ======== */
extern fn Font load_font(char *fileName) @extern("LoadFont");
extern fn Font load_font_ex(char *fileName, CInt fontSize, CInt *codepoints, CInt codepointCount) @extern("LoadFontEx");
extern fn void unload_font(Font font) @extern("UnloadFont");
extern fn Vector2 measure_text_ex(Font font, char *text, float fontSize, float spacing) @extern("MeasureTextEx");

/* ======== Input ======== */
extern fn bool is_key_pressed(CInt key) @extern("IsKeyPressed");

/* ======== Miscellaneous ======== */
extern fn float get_frame_time() @extern("GetFrameTime");
extern fn void set_target_fps(CInt fps) @extern("SetTargetFPS");
extern fn Color get_color(CUInt hexValue) @extern("GetColor");
extern fn void set_trace_log_level(TraceLogLevel logLevel) @extern("SetTraceLogLevel");

