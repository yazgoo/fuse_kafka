#ifndef INPUT_H
#define INPUT_H
#include "config.h"
#include "util.c"
#ifndef TEST
#include "context.c"
#include "output.c"
#else
#include "input_plugin_test.c"
#endif /* TEST */
#include "input.h"
#define requires(lol) /* lol */
#define FUSE_KAFKA_WATCHED_DIRS "/var/run/fuse_kafka/watched"
void input_is_watching_directory(char* path)
{
    char* dir = concat(FUSE_KAFKA_WATCHED_DIRS, path);
    if(dir != NULL) mkdir_p(dir);
    char* pid = integer_concat("", getpid(), ".pid");
    if(pid != NULL)
    {
        char* pid_path = concat(dir, pid);
        if(pid_path != NULL)
        {
            touch(pid_path, "");
            free(pid_path);
        }
        free(pid);
    }
    free(dir);
}
#endif /* INPUT_H */
