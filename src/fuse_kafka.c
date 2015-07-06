/** @file 
 * @brief main fuse_kafka source
 **/ 
#include <dlfcn.h>
#include "version.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdlib.h>
#include "time_queue.c"
#include <sys/stat.h>
#ifndef MINGW_VER
#include <sys/wait.h>
#endif
#include "dynamic_configuration.c"
#include "input.h"
/** @brief declare a configuration item, which is a list of string an
 * and a number of those */
#include "arguments.c"
#include "trace.c"
// global variable used in atexit
config conf;
void configuration_clean()
{
    free_fields_and_tags(&conf);
}
int my_input_setup(int argc, char** argv, int limit)
{
    char* input = "overlay";
    trace_debug("my_input_setup: starting, with %d inputs", conf.input_n);
    if(conf.input_n > 0) input = conf.input[0];
    char*  lib = malloc(strlen(INPUT_PLUGIN_PREFIX) + strlen(input) + 4);
    strcpy(lib, INPUT_PLUGIN_PREFIX);
    strcpy(lib + strlen(INPUT_PLUGIN_PREFIX), input);
    strcpy(lib + strlen(INPUT_PLUGIN_PREFIX) + strlen(input), ".so");
    trace_debug("my_input_setup: trying to open %s", lib);
    void* handle = dlopen(lib, RTLD_LAZY);
    free(lib);
    if(handle == NULL)
    {
        printf("%s\n", dlerror());
        return 1;
    }
    else
    {
        trace_debug("my_input_setup: loading input_setup_internal()");
        input_setup_t f = dlsym(handle, "input_setup_internal");
        if(f == NULL)
        {
            trace_debug("my_input_setup: loading setup function failed");
            printf("%s\n", dlerror());
            return 1;
        }
        trace_debug("my_input_setup: calling setup function");
        return f(limit, argv, &conf);
    }
}
int fuse_kafka_main(int argc, char *argv[])
{
    trace_debug("fuse_kafka_main: starting");
    int i;
    int limit = get_limit(argc, argv);
    atexit(configuration_clean);
    memset(&conf, 0, sizeof(config));
    trace_debug("fuse_kafka_main:  calling parse_arguments(%d - %d - 1, %d + %d + 1, %d)", argc, limit, argv, limit, &conf);
    if(parse_arguments(argc - limit - 1, argv + limit + 1, &conf))
    {
        trace_debug("arguments parsed, input setup");
        my_input_setup(argc, argv, limit);
        trace_debug("input_setup done");
    }
#ifndef MINGW_VER
    wait(NULL);
#endif
    trace_debug("fuse_kafka_main: done");
    return 0;
}
char* cmd = NULL;
#ifdef TEST
#include "test.c"
#else
int main(int argc, char** argv)
{
    trace_debug("main: calling fuse_kafka_main(%d, %d)", argc, argv);
    fuse_kafka_main(argc, argv);
}
#endif
