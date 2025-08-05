// Improve exported video quality
// - https://stackoverflow.com/questions/63460919/how-to-improve-the-output-video-quality-with-ffmpeg-and-h264-videotoolbox-flag
// - https://howik.com/advanced-ffmpeg-techniques-for-better-video-quality

struct Ffmpeg
{
    int pipe;
    int pid;
}

extern fn Ffmpeg *start_rendering_video(char *output_path, usz width, usz height, usz fps) @extern("ffmpeg_start_rendering_video");
extern fn Ffmpeg *start_rendering_audio(char *output_path) @extern("ffmpeg_start_rendering_audio");
extern fn bool end_rendering(Ffmpeg *ffmpeg, bool cancel) @extern("ffmpeg_end_rendering");
extern fn bool send_frame_flipped(Ffmpeg *ffmpeg, void *data, usz width, usz height) @extern("ffmpeg_send_frame_flipped");
extern fn bool send_sound_samples(Ffmpeg *ffmpeg, void *data, usz size) @extern("ffmpeg_send_sound_samples");

