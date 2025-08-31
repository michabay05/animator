#![allow(warnings)]

use std::ffi::{CString, c_char, c_float, c_int, c_uint, c_void};
use std::ptr;

#[link(name = "raylib")]
unsafe extern "C" {
    fn SetConfigFlags(flags: c_uint);
    fn InitWindow(width: c_int, height: c_int, title: *const c_char);
    fn CloseWindow();
    fn WindowShouldClose() -> bool;
    fn SetTargetFPS(fps: c_int);
    fn IsWindowResized() -> bool;
    fn GetScreenWidth() -> c_int;
    fn GetScreenHeight() -> c_int;
    fn GetFrameTime() -> c_float;

    fn BeginDrawing();
    fn EndDrawing();
    fn BeginTextureMode(target: RenderTexture2D);
    fn EndTextureMode();

    fn ClearBackground(color: Color);
    fn DrawTexture(texture: Texture2D, posX: c_int, posY: c_int, tint: Color);
    fn DrawTexturePro(
        texture: Texture2D,
        source: Rectangle,
        dest: Rectangle,
        origin: Vector2,
        rotation: c_float,
        tint: Color,
    );
    fn DrawLineEx(startPos: Vector2, endPos: Vector2, thick: c_float, color: Color);
    fn DrawText(text: *const c_char, posX: c_int, posY: c_int, fontSize: c_int, color: Color);
    fn DrawTextEx(
        font: Font,
        text: *const c_char,
        position: Vector2,
        fontSize: c_float,
        spacing: f32,
        tint: Color,
    );
    fn DrawRectanglePro(rec: Rectangle, origin: Vector2, rotation: c_float, color: Color);
    fn DrawFPS(posX: c_int, posY: c_int);

    fn LoadFontEx(
        fileName: *const c_char,
        fontSize: c_int,
        codepoints: *mut c_int,
        codepointCount: c_int,
    ) -> Font;
    fn UnloadFont(font: Font);
    fn LoadImage(path: *const c_char) -> Image;
    fn UnloadImage(image: Image);
    fn LoadTexture(path: *const c_char) -> Texture;
    fn LoadTextureFromImage(image: Image) -> Texture2D;
    fn LoadImageFromTexture(texture: Texture2D) -> Image;
    fn LoadRenderTexture(width: c_int, height: c_int) -> RenderTexture;
    fn SetTextureFilter(texture: Texture2D, filter: i32);
    fn UnloadTexture(texture: Texture);

    fn IsKeyPressed(key: c_int) -> bool;

    fn GetColor(hexValue: c_uint) -> Color;
    fn SetTraceLogLevel(logLevel: i32);
    fn MeasureTextEx(
        font: Font,
        text: *const c_char,
        fontSize: c_float,
        spacing: c_float,
    ) -> Vector2;
}

macro_rules! to_cstr {
    ($s:expr) => {
        CString::new($s).unwrap().as_ptr()
    };
}

pub fn set_config_flags(flags: &[ConfigFlags]) {
    let mut value = 0u32;
    for f in flags {
        value |= *f as u32;
    }
    unsafe { SetConfigFlags(value) }
}

pub fn init_window(width: i32, height: i32, title: &str) {
    unsafe { InitWindow(width, height, to_cstr!(title)) }
    // let c_t = CString::new(title).unwrap();
    // unsafe { InitWindow(width, height, c_t.as_ptr()) }
}

pub fn close_window() {
    unsafe { CloseWindow() }
}

pub fn window_should_close() -> bool {
    unsafe { WindowShouldClose() }
}

pub fn set_target_fps(fps: i32) {
    unsafe { SetTargetFPS(fps) }
}

pub fn is_window_resized() -> bool {
    unsafe { IsWindowResized() }
}

pub fn get_screen_width() -> i32 {
    unsafe { GetScreenWidth() }
}

pub fn get_screen_height() -> i32 {
    unsafe { GetScreenHeight() }
}

pub fn get_frame_time() -> f32 {
    unsafe { GetFrameTime() }
}

pub fn begin_drawing() {
    unsafe { BeginDrawing() }
}

pub fn end_drawing() {
    unsafe { EndDrawing() }
}

pub fn begin_texture_mode(target: RenderTexture2D) {
    unsafe { BeginTextureMode(target) }
}

pub fn end_texture_mode() {
    unsafe { EndTextureMode() }
}

pub fn clear_background(color: Color) {
    unsafe { ClearBackground(color) }
}

pub fn draw_texture(texture: Texture, pos_x: i32, pos_y: i32, tint: Color) {
    unsafe {
        DrawTexture(texture, pos_x, pos_y, tint);
    }
}

pub fn draw_texture_pro(
    texture: Texture2D,
    source: Rectangle,
    dest: Rectangle,
    origin: Vector2,
    rotation: f32,
    tint: Color,
) {
    unsafe { DrawTexturePro(texture, source, dest, origin, rotation, tint) }
}

pub fn draw_rectangle_pro(rec: Rectangle, origin: Vector2, rotation: f32, color: Color) {
    unsafe { DrawRectanglePro(rec, origin, rotation, color) }
}

pub fn draw_line_ex(start: Vector2, end: Vector2, thickness: f32, color: Color) {
    unsafe { DrawLineEx(start, end, thickness, color) }
}

pub fn draw_text(text: &str, pos_x: i32, pos_y: i32, font_size: i32, color: Color) {
    unsafe { DrawText(to_cstr!(text), pos_x, pos_y, font_size, color) }
}

pub fn draw_text_ex(
    font: Font,
    text: &str,
    position: Vector2,
    font_size: f32,
    spacing: f32,
    tint: Color,
) {
    unsafe { DrawTextEx(font, to_cstr!(text), position, font_size, spacing, tint) }
}

pub fn draw_fps(pos_x: i32, pos_y: i32) {
    unsafe { DrawFPS(pos_x, pos_y) };
}

pub fn is_key_pressed(key: KeyboardKey) -> bool {
    unsafe { IsKeyPressed(key as i32) }
}

pub fn set_trace_log_level(level: TraceLogLevel) {
    unsafe { SetTraceLogLevel(level as i32) }
}

pub fn measure_text_ex(font: Font, text: &str, font_size: f32, spacing: f32) -> Vector2 {
    unsafe { MeasureTextEx(font, to_cstr!(text), font_size, spacing) }
}

// -- Structures --
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Vector2 {
    pub x: f32,
    pub y: f32,
}

impl Vector2 {
    pub fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }

    pub fn add(a: Self, b: Self) -> Self {
        Self {
            x: a.x + b.x,
            y: a.y + b.y,
        }
    }

    pub fn subtract(a: Self, b: Self) -> Self {
        Self {
            x: a.x - b.x,
            y: a.y - b.y,
        }
    }

    pub fn multiply(a: Self, b: Self) -> Self {
        Self {
            x: a.x * b.x,
            y: a.y * b.y,
        }
    }

    pub fn scale(v: Self, scalar: f32) -> Self {
        Self {
            x: v.x * scalar,
            y: v.y * scalar,
        }
    }

    pub fn zero() -> Self {
        Self { x: 0.0, y: 0.0 }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl Color {
    pub const WHITE: Self = Self::from_hex(0xffffffff);
    pub const BLACK: Self = Self::from_hex(0x000000ff);
    pub const BEIGE_LT: Self = Self::from_hex(0xf5ebe0ff);
    pub const RED: Self = Self {
        r: 230,
        g: 41,
        b: 55,
        a: 255,
    };
    pub const BLUE: Self = Self {
        r: 0,
        g: 121,
        b: 241,
        a: 255,
    };

    pub const fn from_hex(hex: u32) -> Self {
        // Taken from (https://github.com/raysan5/raylib/blob/master/src/rtextures.c)
        Self {
            r: (hex >> 24) as u8 & 0xff,
            g: (hex >> 16) as u8 & 0xff,
            b: (hex >> 8) as u8 & 0xff,
            a: hex as u8 & 0xff,
        }
    }
}

#[repr(C)]
pub struct Rectangle {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

// NOTE: the actual enum in raylib contains a lot of values
// I am only including the ones that are useful for this project
#[repr(C)]
#[derive(Debug)]
enum PixelFormat {
    Uncompressedr8g8b8a8 = 7,
}

#[repr(C)]
#[derive(Debug)]
pub struct Image {
    pub data: *const c_void,
    pub width: i32,
    pub height: i32,
    mipmaps: i32,
    format: PixelFormat,
}

impl Image {
    pub fn from_path(path: &str) -> Self {
        unsafe { LoadImage(to_cstr!(path)) }
    }

    pub fn from_texture(texture: Texture) -> Self {
        unsafe { LoadImageFromTexture(texture) }
    }

    pub fn unload(self) {
        unsafe { UnloadImage(self) }
    }
}

#[repr(C)]
#[derive(Clone, Debug)]
pub struct Texture {
    id: u32,
    pub width: i32,
    pub height: i32,
    mipmaps: i32,
    format: i32,
}

impl Texture {
    pub fn from_path(path: &str) -> Self {
        unsafe { LoadTexture(to_cstr!(path)) }
    }

    pub fn from_image(image: Image) -> Self {
        unsafe { LoadTextureFromImage(image) }
    }

    pub fn unload(self) {
        unsafe { UnloadTexture(self) }
    }

    pub fn set_filter(&self, filter: TextureFilter) {
        unsafe { SetTextureFilter(self.clone(), filter as i32) }
    }
}

type Texture2D = Texture;

#[repr(C)]
pub enum TextureFilter {
    Point = 0,
    Bilinear,
}

#[derive(Clone, Debug)]
#[repr(C)]
pub struct RenderTexture {
    id: u32,
    pub texture: Texture,
    depth: Texture,
}

impl RenderTexture {
    pub fn new(width: i32, height: i32) -> Self {
        unsafe { LoadRenderTexture(width, height) }
    }
}

// RenderTexture2D, same as RenderTexture
type RenderTexture2D = RenderTexture;

#[repr(C)]
pub enum TraceLogLevel {
    ALL,
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    NONE,
}

#[repr(C)]
struct GlyphInfo {
    value: i32,
    offset_x: i32,
    offset_y: i32,
    advance_x: i32,
    image: Image,
}

#[repr(C)]
#[derive(Clone, Debug)]
pub struct Font {
    pub base_size: i32,
    glyph_count: i32,
    glyph_padding: i32,
    pub texture: Texture2D,
    recs: *mut Rectangle,
    glyphs: *mut GlyphInfo,
}

impl Font {
    pub fn from_path(path: &str, font_size_ratio: f32) -> Self {
        let min_side = if get_screen_width() > get_screen_height() {
            get_screen_height()
        } else {
            get_screen_width()
        } as f32;
        let size = (font_size_ratio * min_side) as i32;
        unsafe { LoadFontEx(to_cstr!(path), size, ptr::null_mut(), 0) }
    }

    pub fn reload(&mut self, font_path: &str, font_size_ratio: f32) {
        self.clone().unload();
        *self = Font::from_path(font_path, font_size_ratio);
    }

    pub fn unload(self) {
        unsafe { UnloadFont(self) }
    }
}

#[derive(Copy, Clone)]
pub enum ConfigFlags {
    Msaa4xHint = 0x00000020,
    WindowResizable = 0x00000004,
}

#[repr(C)]
pub enum KeyboardKey {
    Space = 32,
    Minus = 45,
    Equal = 61,
    R = 82,
}
