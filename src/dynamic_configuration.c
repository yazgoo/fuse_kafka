#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#ifdef TEST
#define FUSE_KAFKA_DYNAMIC_CONFIGURATION_PATH "/tmp/fuse_kafka.args"
#else
#define FUSE_KAFKA_DYNAMIC_CONFIGURATION_PATH "/var/run/fuse_kafka.args"
#endif
typedef struct
{
    int argc;
    char** argv;
    char* line;
    int loaded;
    void* context;
    char* path;
    pthread_t thread;
} dynamic_configuration;
/**
 * @return current dynamic configuration
 */
static dynamic_configuration* dynamic_configuration_get()
{
    static dynamic_configuration conf;
    return &conf;
}
static char* dynamic_configuration_get_path()
{
    dynamic_configuration* conf = dynamic_configuration_get();
    if(conf->path == NULL)
        return FUSE_KAFKA_DYNAMIC_CONFIGURATION_PATH;
    return conf->path;
}
/**
 * @brief gets a file in one single string, that you will have to free
 * @param path the file to parse
 * @param linep a pointer to a string which will hold the line, will be allocated
 * @param sizep a pointer to hold the size of the string with the line
 * @return 0 if sucesseeded, otherwise there is nothing to free
 **/
int parse_line_from_file(char* path, char** linep, int* sizep)
{
    FILE* f = fopen(path, "r");
    if(f == NULL) return 1;
    fseek(f, 0, SEEK_END);
    (*sizep) = ftell(f);
    (*linep) = (char*) malloc((*sizep) + 1);
    if((*linep) == NULL) return 2;
    fseek(f, 0, SEEK_SET);
    int read = fread((*linep), 1, (*sizep), f);
    fclose(f);
    if(read < (*sizep)) { free((*linep)); return 3; }
    (*linep)[*sizep] = 0;
    return 0;
}
/**
 * @brief parses a file, giving back the line parsed and an array pointing to this line
 * @param path the file to parse
 * @param argcp a pointer to hold the array size of parsed argument
 * @param argvp a pointer to hold the array of parsed argument, to be freed once used
 * @param linep a pointer to a string which will hold the line, to free once used
 * @return 0 if sucesseeded, otherwise there is nothing to free
 */
int parse_args_from_file(char* path, int* argcp, char*** argvp, char** linep)
{
    int i = *argcp = 0, k = 0, size = 0;
    if((i = parse_line_from_file(path, linep, &size)) != 0) return i;
    for(i = 0; i < size; i++)
        if((i == 0 || (*linep)[i-1]  == ' ') && (*linep)[i] != ' ') (*argcp)++;
    (*argvp) = calloc((*argcp), sizeof(char*));
    if((*argvp) == NULL) { free((*linep)); return 4; }
    for(i = 0; i < size; i++)
        if((i == 0 || !(*linep)[i-1] || (*linep)[i-1]  == ' ') && (*linep)[i] != ' ')
        {
            (*argvp)[k++] = (*linep) + i;
            if(i > 0) (*linep)[i - 1] = 0;
        }
    return 0;
}
/**
 * @return 1 if the configuration file has changed since last check, 0 otherwise
 */
int dynamic_configuration_changed()
{
    static time_t last_change = 0;
    struct stat stats;
    if(last_change == 0) time(&last_change);
    if(stat(dynamic_configuration_get_path(), &stats) != 0)
        return 0;
    if(stats.st_mtime <= last_change) return 0;
    last_change = stats.st_mtime;
    return 1;
}
/**
 * @brief cleanups current dynamic configuration allocations
 */
void dynamic_configuration_free()
{
    dynamic_configuration* conf = dynamic_configuration_get();
    free(conf->line);
    free(conf->argv);
}
/**
 * @brief loads dynamic configuration from dynamic configuration file
 */
int dynamic_configuration_load()
{
    char* line;
    char** argv;
    int argc;
    dynamic_configuration* conf = dynamic_configuration_get();
    if(parse_args_from_file(dynamic_configuration_get_path(),
                &argc, &argv, &line) == 0)
    {
        if(conf->loaded) dynamic_configuration_free();
        conf->argc = argc;
        conf->line = line;
        conf->argv = argv;
        conf->loaded = 1;
        return 0;
    }
    return 1;
}
/**
 * @brief on every dynamic configuration change, calls f
 * @param f the routine which will be called with parsed arguments
 */
void* dynamic_configuration_watch_routine(void(*f)(int argc, char** argv, void* context))
{
    while(1)
    {
        if(dynamic_configuration_changed() && dynamic_configuration_load() == 0)
        {
            dynamic_configuration* conf = dynamic_configuration_get();
            f(conf->argc, conf->argv, conf->context);
        }
        sleep(5);
    }
    return NULL;
}
/**
 * @brief start a watcher thread running dynamic_configuration_watch_routine
 */
void dynamic_configuration_watch(void(*f)(int argc, char** argv, void* context))
{
    pthread_create(&(dynamic_configuration_get()->thread), NULL, 
            (void * (*)(void *))
            dynamic_configuration_watch_routine, (void*) f);
}
void dynamic_configuration_watch_stop()
{
    pthread_cancel(dynamic_configuration_get()->thread);
    pthread_join(dynamic_configuration_get()->thread, NULL);
}
