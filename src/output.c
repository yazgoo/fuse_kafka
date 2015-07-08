#include "version.h"
#include "output.h"
#include "time_queue.c"
#ifndef TEST
#include "dynamic_configuration.c"
#include "arguments.c"
#endif
#include "plugin.c"
#ifdef _WIN32
struct group {
    char* gr_name;
};
struct passwd {
    char* pw_name;
};
void* getgrgid(int i)
{
    return NULL;
}
void* getpwuid(int i)
{
    return NULL;
}
#else
#include <grp.h>
#include <pwd.h>
#endif
/**
 * @brief actually does the write to kafka of a string with the given
 * file path
 * @param path file path to save to kafka
 * @param buf write buffer
 * @param size size of the buffer to write
 * @param offset starting point in the buffer
 * @return 0 if the write succeeded, 1 otherwise
 **/
static int actual_kafka_write(const char* prefix, const char *path, const char *buf,
        size_t size, off_t offset)
{
    char* ret = NULL;
    (void) path;
    char timestamp[] = "YYYY-MM-ddTHH:mm:ss.SSS+0000                               ";
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
    asprintf(&ret, format, prefix,
            path + 1, context->pid, context->uid, context->gid,
            text, timestamp, user, group, command, VERSION,
            conf->fields_s, conf->tags_s);
    free(command);
    free(text);
    if (ret == NULL) {
        fprintf(stderr, "Error in asprintf\n");
        return 1;
    }
    trace_debug("actual_kafka_write: calling my_output_send()");
    int r = my_output_send(context->private_data, ret, strlen(ret));
    trace_debug("actual_kafka_write: my_output_send result == %d", r);
    free(ret);
    return 0;
}
#include "trace.c"
/**
 * @brief checks if writes from the given path should be written to
 * kafka
 * @param path the write path
 * @param size the write size
 **/
static int should_write_to_kafka(const char* path, size_t size)
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    trace_debug("should_write_to_kafka: private_data %x", private_data);
    if(private_data == NULL) return 0;
    trace_debug("should_write_to_kafka: private_data->rkt %x", private_data->rkt);
    if(private_data->rkt == NULL) return 0;
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
    if(conf->quota_queue == NULL) return 1;
    if(time_queue_overflows(conf->quota_queue, (char*)path, size)) i = 0;
    else i = 1;
    time_queue_set(conf->quota_queue, (char*)path);
    return i;
}
void output_write(const char *prefix, const char *path, const char *buf,
        size_t size, off_t offset)
{
    if(should_write_to_kafka(path, size))
        actual_kafka_write(prefix, path, buf, size, offset);
}
#define PLUGIN_FUNCTION_GETTER(name)\
name##_t* get_##name()\
{\
    static name##_t function_ptr = 0; \
    return &function_ptr;\
}
#define PLUGIN_FUNCTION(name)\
    trace_debug("PLUGIN_FUNCTION: getting " STR(name));\
    name##_t ptr = *(get_##name());\
    trace_debug("PLUGIN_FUNCTION: got " STR(name) ": %x", ptr);\
    if(ptr == NULL) return 1; return (*ptr)
#define PLUGIN_FUNCTION_LOAD(name)\
    trace_debug("my_output_setup: load_function_from_plugin("STR(name)\
            ") result %x", f);\
    *(get_##name()) = (name##_t) load_function_from_plugin(\
            handle, STR(name));\
    trace_debug("my_output_setup: load_function_from_plugin("STR(name)\
            ") result %x",\
            get_output_send());
PLUGIN_FUNCTION_GETTER(output_send)
PLUGIN_FUNCTION_GETTER(output_clean)
PLUGIN_FUNCTION_GETTER(output_update)
int my_output_send(kafka_t* k, char* buf, size_t len)
{ PLUGIN_FUNCTION(output_send)(k, buf, len); }
int my_output_clean(kafka_t* k) { PLUGIN_FUNCTION(output_clean)(k); }
int my_output_update() { PLUGIN_FUNCTION(output_update)(); }
void output_destroy(void* untyped)
{
    kafka_t* k = (kafka_t*) untyped;
    if(k == NULL) return;
    if(k->conf->quota_n > 0) time_queue_delete(k->conf->quota_queue);
    if(k->zhandle != NULL) zookeeper_close(k->zhandle);
    my_output_clean(k);
    free(k);
    dynamic_configuration_watch_stop();
}
void setup_from_dynamic_configuration(int argc, char** argv, void* context)
{
    kafka_t* k = (kafka_t*) context;
    memset(k->conf, 0, sizeof(config));
    parse_arguments(argc, argv, k->conf);
    if(k->zhandle != NULL)
    {
        zookeeper_close(k->zhandle);
        k->zhandle = NULL;
    }
    my_output_update();
}
int my_output_setup(config* conf, void* k)
{
    char* output = "kafka";
    if(conf->output_n > 0) output = conf->output[0];
    trace_debug("my_output_setup: loading output plugin %s", output);
    void* handle = load_plugin(OUTPUT_PLUGIN_PREFIX, output);
    trace_debug("my_output_setup: load_plugin result %x", handle);
    output_setup_t f = (output_setup_t*) load_function_from_plugin(handle, "output_setup");
    PLUGIN_FUNCTION_LOAD(output_send)
    PLUGIN_FUNCTION_LOAD(output_update)
    PLUGIN_FUNCTION_LOAD(output_clean)
    if(f != NULL) return f(k, conf);
    trace_debug("my_output_setup: output_setup is NULL");
    return 1;
}
void* output_init(config* conf)
{
    trace_debug("output_init: entry");
    fuse_get_context()->private_data = (void*) conf;
    dynamic_configuration_watch(&setup_from_dynamic_configuration);
    trace_debug("output_init: watching dynamic configuration");
    int directory_fd = conf->directory_fd;
    int time_queue_size;
    fchdir(directory_fd);
    close(directory_fd);
    kafka_t* k = (kafka_t*) malloc(sizeof(kafka_t));
    memset(k, 0, sizeof(kafka_t));
    trace_debug("output_init: calling my_output_setup");
    if(my_output_setup(conf, (void*) k))
    {
        printf("output_init: output_setup failed\n");
        return NULL;
    }
    k->conf = conf;
    if(conf->quota_n > 0)
    {
        time_queue_size = conf->quota_n > 1 ? atoi(conf->quota[1]):20;
        conf->quota_queue = time_queue_new(
                time_queue_size, atoi(conf->quota[0]));
    }
    dynamic_configuration_get()->context = (void*) k;
    fuse_get_context()->private_data = (void*) k;
    return (void*) k;
}
void input_setup_internal(int argc, char** argv, void* conf)
{
    fuse_get_context()->private_data = (void*) output_init((config*) conf);
#ifndef TEST
    input_setup(argc, argv, conf);
#endif
}
