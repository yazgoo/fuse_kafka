#define VERSION "0.1.3"
#define FUSE_USE_VERSION 26
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define _GNU_SOURCE
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <pwd.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdlib.h>
#include "time_queue.c"
#include <grp.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define CONFIG_ITEM(name) char** name; size_t name ## _n;
typedef struct _config {
    int directory_fd;
    size_t directory_n;
    char* fields_s;
    char* tags_s;
    CONFIG_ITEM(directories)
    CONFIG_ITEM(persist)
    CONFIG_ITEM(excluded_files)
    CONFIG_ITEM(substitutions)
    CONFIG_ITEM(brokers)
    CONFIG_ITEM(topic)
    CONFIG_ITEM(fields)
    CONFIG_ITEM(tags)
} config;
#define XSTR(s) STR(s)
#define STR(s) #s
#define CONFIG_CURRENT(expected) if(!strcmp(name, STR(expected))) \
{ printf("parsing " STR(expected) "\n");current_size = &(conf->expected ## _n); conf->expected = argv + i + 1; }
#include "util.c"
#include "kafka_client.c"
static int actual_kafka_write(const char *path, const char *buf,
        size_t size, off_t offset)
{
    char* ret = NULL;
    (void) path;
    char timestamp[] = "YYYY-MM-ddTHH:mm:ss.SSS+0000";
    char* text = base64(buf, size);
    struct fuse_context* context = fuse_get_context();
    struct group* sgroup = getgrgid(context->gid);
    struct passwd* suser = getpwuid(context->uid);
    char* user = suser == NULL ? "":suser->pw_name;
    char* group = sgroup == NULL ? "":sgroup->gr_name;
    char* command = get_command_line(context->pid);
    char* format = "{\"path\": \"%s%s\", \"pid\": %d, \"uid\": %d, "
        "\"gid\": %d, \"@message\": \"%s\", \"@timestamp\": \"%s\","
        "\"user\": \"%s\", \"group\": \"%s\", \"command\": \"%s\","
        "\"@version\": \"%s\", \"@fields\": %s, \"@tags\": %s}";
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    config* conf = (config*)private_data->conf;
    set_timestamp(timestamp);
    asprintf(&ret, format, conf->directories[conf->directory_n],
            path + 1, context->pid, context->uid, context->gid,
            text, timestamp, user, group, command, VERSION,
            conf->fields_s, conf->tags_s);
    free(command);
    free(text);
    if (ret == NULL) {
        fprintf(stderr, "Error in asprintf\n");
        return 1;
    }
    send_kafka(context->private_data, ret, strlen(ret));
    free(ret);
    return 0;
}
#include "trace.c"
static int should_write_to_kafka(const char* path)
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    config* conf = (config*)private_data->conf;
    int i = 0;
    for(i = 0; i < conf->excluded_files_n; i++)
    {
        char* pattern = conf->excluded_files[i];
        if(!fnmatch(pattern, path, 0))
        {
            return 0;
        }
    }
    return 1;
}
static int kafka_write(const char *path, const char *buf,
        size_t size, off_t offset, struct fuse_file_info *fi)
{
    int res;
    if(should_write_to_kafka(path) &&
            actual_kafka_write(path, buf, size, offset)) return 1;
    res = pwrite(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;

    return res;
}
#include "overlay.c"
int get_limit(int argc, char** argv)
{
    int i = 0;
    for(; i < argc; i++) if(!strcmp(argv[i], "--")) break;
    return i;
}
char* array_to_container_string(char** array, size_t n, char open_char,
        char close_char, char sep1, char sep2)
{
    int i = 0;
    char* str = (char*) malloc(3);
    int k = sprintf(str, "%c", open_char);
    if(array != NULL)
        for(i = 0; i < n; i++)
        {
            str = realloc(str, k + 1 + strlen(array[i]) + 2 + 2);
            k += sprintf(str + k, "\"%s\"", array[i]);
            if(i != n-1) k += sprintf(str + k, "%c ", i % 2 ? sep2 : sep1);
        }
    sprintf(str + k, "%c", close_char);
    return str;
}
void add_fields_and_tags(config* conf)
{
    conf->fields_s = array_to_container_string(
            conf->fields, conf->fields_n, '{', '}', ':', ',');
    printf("fields: %s\n", conf->fields_s);
    conf->tags_s = array_to_container_string(
            conf->tags, conf->tags_n, '[', ']', ',', ',');
    printf("tags: %s\n", conf->tags_s);
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
            else CONFIG_CURRENT(brokers)
            else CONFIG_CURRENT(topic)
            else CONFIG_CURRENT(fields)
            else CONFIG_CURRENT(tags)
            else
            {
                printf("unknown option %s\n", argv[i]);
                return 0;
            }
            *current_size = 0;
        }
        else
        {
            printf("\t- %s\n", argv[i]);
            (*current_size)++;
        }
    }
    add_fields_and_tags(conf);
    return 1;
}
int fuse_kafka_main(int argc, char *argv[])
{
    int i;
    int limit = get_limit(argc, argv);
    config conf;
    memset(&conf, 0, sizeof(config));
    if(parse_arguments(argc - limit - 1, argv + limit + 1, &conf))
    {
        for(conf.directory_n = 0; conf.directory_n < conf.directories_n;
                conf.directory_n++)
        {
            argv[1] = conf.directories[conf.directory_n];
            //if(!fork())
            {
                conf.directory_fd = open(conf.directories[conf.directory_n],
                        O_RDONLY);
                return fuse_main(limit, argv, &kafka_oper, &conf);
            }
        }
    }
    wait(NULL);
    return 0;
}
char* cmd = NULL;
#define RET_CMD(...) { asprintf(&cmd, __VA_ARGS__); system(cmd); free(cmd); }; return 0;
#ifdef TEST
#include "test.c"
#else
int main(int argc, char** argv)
{
    fuse_kafka_main(argc, argv);
}
#endif
