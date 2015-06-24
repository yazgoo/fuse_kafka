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
#include <fnmatch.h>
#include <stdarg.h>
#include <stdlib.h>
#include "time_queue.c"
#include <sys/stat.h>
#include <sys/wait.h>
#include "dynamic_configuration.c"
#include "input.h"
/** @brief declare a configuration item, which is a list of string an
 * and a number of those */
#include "arguments.c"
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
    char*  lib = malloc(strlen(INPUT_PLUGIN_PREFIX) + strlen(input) + 4);
    strcpy(lib, INPUT_PLUGIN_PREFIX);
    strcpy(lib + strlen(INPUT_PLUGIN_PREFIX), input);
    strcpy(lib + strlen(INPUT_PLUGIN_PREFIX) + strlen(input), ".so");
    void* handle = dlopen(lib, RTLD_LAZY);
    free(lib);
    if(handle == NULL)
    {
        printf("%s\n", dlerror());
        return 1;
    }
    else
    {
        input_setup_t f = dlsym(handle, "input_setup_internal");
        return f(limit, argv, &conf);
    }
}
int fuse_kafka_main(int argc, char *argv[])
{
    int i;
    int limit = get_limit(argc, argv);
    atexit(configuration_clean);
    memset(&conf, 0, sizeof(config));
    if(parse_arguments(argc - limit - 1, argv + limit + 1, &conf))
    {
        my_input_setup(argc, argv, limit);
    }
    wait(NULL);
    return 0;
}
char* cmd = NULL;
#ifdef TEST
#include "test.c"
#else
int main(int argc, char** argv)
{
    fuse_kafka_main(argc, argv);
}
#endif
