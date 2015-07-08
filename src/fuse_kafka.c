/** @file 
 * @brief main fuse_kafka source
 **/ 
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
#include "output.h"
#include "plugin.c"
#include "input.h"
// global variable used in atexit
config conf;
void configuration_clean()
{
    free_fields_and_tags(&conf);
}
int my_input_setup(int argc, char** argv, int limit)
{
    char* input = "overlay";
    if(conf.input_n > 0) input = conf.input[0];
    input_setup_t f = (input_setup_t*) load_plugin_function(INPUT_PLUGIN_PREFIX, input, "input_setup_internal");
    if(f != NULL) return f(limit, argv, &conf);
    return 1;
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
