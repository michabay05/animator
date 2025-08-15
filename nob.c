#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    nob_cc(&cmd);
    nob_cc_flags(&cmd);
    nob_cmd_append(&cmd, "-Werror");
    nob_cc_inputs(&cmd,
        "src/main.c", "src/animator.h",
        "src/animator.c", "./src/ffmpeg_linux.c"
    );
    nob_cc_output(&cmd, "main");
    nob_cmd_append(&cmd, "-L./raylib/lib/", "-l:libraylib.a", "-lm");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "./main");
    if (!cmd_run_sync_and_reset(&cmd)) return 1;

    return 0;
}
