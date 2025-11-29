#include "span.h"
#include <stdint.h>

Arena arena = {0};

void spu_print_err(void *umka)
{
    UmkaError *err = umkaGetError(umka);
    printf("%s:%d:%d: %s\n", err->fnName, err->line, err->pos, err->msg);
}

bool __spu_call_fn(void *umka, const char *fn_name, UmkaStackSlot **slot, size_t storage_bytes)
{
    UmkaFuncContext fn = {0};
    bool umkaOk = umkaGetFunc(umka, NULL, fn_name, &fn);
    if (!umkaOk)
        return false;

    if (storage_bytes > 0) {
        umkaGetResult(fn.params, fn.result)->ptrVal = arena_alloc(&arena, storage_bytes);
    }

    umkaOk = umkaCall(umka, &fn) == 0;
    if (!umkaOk) {
        spu_print_err(umka);
        return false;
    }

    // If slot is null, then I probably don't care about the result.
    // Just the fact that the function ran
    if (slot != NULL) {
        *slot = umkaGetResult(fn.params, fn.result);
    }
    return true;
}

void spu_run_sequence(void *umka)
{
    __spu_call_fn(umka, "sequence", NULL, 0);
}

bool __spu_get_tasks(void *umka, TaskList *tasks)
{
    UmkaStackSlot *slot = NULL;
    __spu_call_fn(umka, "sequence", NULL, 0);
    printf("Sequence done...\n");

    slot = NULL;
    __spu_call_fn(umka, "get_task_count", &slot, 0);
    int n = (int)slot->intVal;
    printf("n = %d\n", n);

    slot = NULL;
    __spu_call_fn(umka, "get_tasks", &slot, n * sizeof(UmkaTask));

    UmkaTask *dt = (UmkaTask *)slot->ptrVal;
    TaskList tl = {0};

    for (int j = 0; j < n; j++) {
        UmkaTask ut = dt[j];
        Task t = {0};
        t.duration = ut.duration;

        for (int i = 0; i < umkaGetDynArrayLen(&ut.actions); i++) {
            Action a = {
                .obj_id = ut.actions.data[i].obj_id,
                .kind = ut.actions.data[i].kind,
                .delay = ut.actions.data[i].delay,
            };
            arena_da_append(&arena, &t.actions, a);
        }

        arena_da_append(&arena, &tl, t);
    }

    *tasks = tl;
    return true;
}

void sp_print_tasks(TaskList tl)
{
    printf("%d\n", tl.count);
    for (int i = 0; i < tl.count; i++) {
        Task t = tl.items[i];
        printf("Task {\n");
        printf("    duration = %f\n", t.duration);
        for (int k = 0; k < t.actions.count; k++) {
            Action a = t.actions.items[k];
            printf("    [%2d] {id = %d, kind = %d, delay = %f}\n", k, a.obj_id, a.kind, a.delay);
        }
        printf("}\n");
    }
}

bool spu_content_preamble(Arena *arena, const char *filename, char **content)
{
    char preamble[2*1024] = {0};

    FILE *fp = fopen("preamble.um", "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not find '%s'\n", filename);
        return false;
    }

    // Source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0L, SEEK_END);
    size_t fsz = ftell(fp);
    rewind(fp);
    fread(preamble, fsz, 1, fp);

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Could not find '%s'\n", filename);
        return false;
    }

    fseek(fp, 0L, SEEK_END);
    fsz = ftell(fp);
    rewind(fp);

    char *file_content = arena_alloc(arena, fsz + 1);
    fread(file_content, fsz, 1, fp);
    file_content[fsz] = '\0';

    *content = arena_sprintf(arena, "%s\n%s", preamble, file_content);
    return true;
}

Vector2 sp_from_dv2(DVector2 dv)
{
    return (Vector2){
        .x = (f32)dv.x,
        .y = (f32)dv.y,
    };
}

DVector2 sp_to_dv2(Vector2 v)
{
    return (DVector2){
        .x = (f64)v.x,
        .y = (f64)v.y,
    };
}
