#define NOB_IMPLEMENTATION
#include "nob.h"

#define SRCDIR "src/"
#define OBJDIR "obj/"
#define BINARY "span.bin"
#define VENDORDIR "vendor/"
#define VENDOR_INCDIR VENDORDIR"include"
#define VENDOR_LIBDIR VENDORDIR"lib"
#define CC "gcc"

static Nob_Cmd cmd = {0};

void compile_objs(const char *src_name)
{
    nob_cmd_append(&cmd, CC);
    nob_cmd_append(&cmd, "-Wall", "-Wextra", "-pedantic", "-I"VENDOR_INCDIR);
    nob_cmd_append(&cmd, "-ggdb");
    nob_cmd_append(&cmd, "-c");
    nob_cmd_append(&cmd, nob_temp_sprintf(SRCDIR"%s.c", src_name));
    nob_cmd_append(&cmd, "-o");
    nob_cmd_append(&cmd, nob_temp_sprintf(OBJDIR"%s.o", src_name));
}

void link_objs(size_t n, const char *src_names[n])
{
    nob_cmd_append(&cmd, CC);
    for (size_t i = 0; i < n; i++) {
        nob_cmd_append(&cmd, nob_temp_sprintf(OBJDIR"%s.o", src_names[i]));
    }
    nob_cmd_append(&cmd, "-o", BINARY);
    nob_cmd_append(&cmd, "-L"VENDOR_LIBDIR);
    nob_cmd_append(&cmd, "-l:libraylib.a", "-lm");
    nob_cmd_append(&cmd, "-l:libumka.a");
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    nob_shift_args(&argc, &argv);
    bool run_bin = false;
    if (argc > 0) {
        if (!strncmp(argv[0], "run", 3)) {
            run_bin = true;
        } else {
            nob_log(NOB_ERROR, "Wrong usage. Will document at some point...");
            return 1;
        }
    }

    const char *src_names[] = { "main", "ffmpeg_linux", "span" };

    size_t n = NOB_ARRAY_LEN(src_names);
    for (size_t i = 0; i < n; i++) {
        cmd.count = 0;
        compile_objs(src_names[i]);
        if (!nob_cmd_run_sync(cmd)) return 1;
    }
    cmd.count = 0;
    link_objs(n, src_names);
    if (!nob_cmd_run_sync(cmd)) return 1;

    if (run_bin) {
        cmd.count = 0;
        nob_cmd_append(&cmd, "./"BINARY);
        if (!nob_cmd_run_sync(cmd)) return 1;
    }
    return 0;
}
