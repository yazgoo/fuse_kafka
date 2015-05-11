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
#include "config.h"
/** @brief convert a symbol to a string */
#define STR(s) #s
/** @brief check if name matches a configuration name, if so points
 * current_size pointer to the size of the matched configuration and
 * sets the address of the string array for that configuration to next
 * item in argv list */
#define CONFIG_CURRENT(expected) \
    if(!strcmp(name, STR(expected))) { \
        current_size = &(conf->expected ## _n); \
        conf->expected = argv + i + 1; \
    }
#include "util.c"
void add_fields_and_tags(config* conf)
{
    if(conf->fields_s != NULL) free(conf->fields_s);
    conf->fields_s = array_to_container_string(
            conf->fields, conf->fields_n, '{', '}', ':', ',');
    if(conf->tags_s != NULL) free(conf->tags_s);
    conf->tags_s = array_to_container_string(
            conf->tags, conf->tags_n, '[', ']', ',', ',');
}
void free_fields_and_tags(config* conf)
{
    if(conf->fields_s != NULL) free(conf->fields_s);
    if(conf->tags_s != NULL) free(conf->tags_s);
}
int parse_arguments(int argc, char** argv, config* conf)
{
    int i;
    size_t* current_size;
    char* name;
    for(i = 0; i < argc; i++)
    {
        if(strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-')
        {
            name = argv[i] + 2;
            CONFIG_CURRENT(directories)
            else CONFIG_CURRENT(persist)
            else CONFIG_CURRENT(excluded_files)
            else CONFIG_CURRENT(substitutions)
            else CONFIG_CURRENT(zookeepers)
            else CONFIG_CURRENT(brokers)
            else CONFIG_CURRENT(topic)
            else CONFIG_CURRENT(fields)
            else CONFIG_CURRENT(tags)
            else CONFIG_CURRENT(quota)
            else CONFIG_CURRENT(input)
            else
            {
                printf("unknown option %s\n", argv[i]);
                return 0;
            }
            *current_size = 0;
        }
        else
        {
            (*current_size)++;
        }
    }
    add_fields_and_tags(conf);
    return 1;
}
// global variable used in atexit
config conf;
void configuration_clean()
{
    free_fields_and_tags(&conf);
}
int fuse_kafka_main(int argc, char *argv[])
{
    int i;
    int limit = get_limit(argc, argv);
    atexit(configuration_clean);
    memset(&conf, 0, sizeof(config));
    if(parse_arguments(argc - limit - 1, argv + limit + 1, &conf))
    {
        for(conf.directory_n = 0; conf.directory_n < conf.directories_n;
                conf.directory_n++)
        {
            argv[1] = conf.directories[conf.directory_n];
            if(!fork())
            {
#ifdef TEST
                break;
#endif
                conf.directory_fd = open(conf.directories[conf.directory_n],
                        O_RDONLY);
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
        }
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
