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

enum TraceLogLevel  {
    ALL,      // Display all logs
    TRACE,    // Trace logging, intended for internal use only
    DEBUG,    // Debug logging, used for internal debugging, it should be disabled on release builds
    INFO,     // Info logging, used for program execution info
    WARNING,  // Warning logging, used on recoverable failures
    ERROR,    // Error logging, used on unrecoverable failures
    FATAL,    // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    NONE      // Disable logging
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

/* ======== Miscellaneous ======== */
extern fn float get_frame_time() @extern("GetFrameTime");
extern fn void set_target_fps(CInt fps) @extern("SetTargetFPS");
extern fn Color get_color(CUInt hexValue) @extern("GetColor");
extern fn void set_trace_log_level(TraceLogLevel logLevel) @extern("SetTraceLogLevel");

