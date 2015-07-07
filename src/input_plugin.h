#ifndef INPUT_H
#define INPUT_H
#include "config.h"
#include "util.c"
#ifndef TEST
#include "context.c"
#include "output.c"
#else
#include "input_plugin_test.c"
#include "trace.c"
#endif /* TEST */
#include "input.h"
#define requires(lol) /* lol */
#define target(lol) /* lol */
#define FUSE_KAFKA_WATCHED_DIRS "/var/run/fuse_kafka/watched"
static inline char** input_get_last_watching_directory()
{
    static char* path;
    return &path;
}
void input_is_watching_directory(char* path)
{
    char* dir = concat(FUSE_KAFKA_WATCHED_DIRS, path);
    if(dir != NULL) mkdir_p(dir);
    char* pid = integer_concat("", getpid(), ".pid");
    trace_debug("input_is_watching_directory: trying to declare %s",
            pid);
    *(input_get_last_watching_directory()) = path;
    if(pid != NULL)
    {
        char* pid_path = concat(dir, pid);
        trace_debug("input_is_watching_directory: trying to touch %s",
                pid_path);
        if(pid_path != NULL)
        {
            if(!touch(pid_path, ""))
                trace_error("input_is_watching_directory:"
                        "failed opening %s", pid_path);
            free(pid_path);
        }
        else
            trace_error("input_is_watching_directory: no pid found for %s",
                    path);
        free(pid);
    }
    else
        trace_error("input_is_watching_directory: no pid found for %s",
                path);
    free(dir);
}
#endif /* INPUT_H */
