/** @file 
 * @brief Utility to send trace from fuse_kafka into kafka 
 **/ 
/**
 * @brief formats and send traces to kafka, behaving like printf
 * @param fmt the format of the string to send as defined in printf
 * @return 0 on success
 **/ 
#ifndef TRACE_C
#define TRACE_C
#include "trace.h" 
#include "context.c"
int trace_debug_enabled()
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    if(private_data == NULL) return 0;
    config* conf = (config*)private_data->conf;
    return (conf->debug_n > 0);
}
char* trace_log_path_get()
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    if(private_data == NULL) return NULL;
    config* conf = (config*)private_data->conf;
    printf("%d\n", (int) conf->log_n);
    if(conf->log_n != 1) return NULL;
    return conf->log[0];
}
static int trace(const char* fmt, ...)
{
    char* str;
    int res;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&str, fmt, ap);
    va_end(ap);
    //res = actual_kafka_write("./fuse_kafka.log", str, strlen(str), 0);
    char* path = trace_log_path_get();
    if(path == NULL)
    {
        printf("%s\n", str);
    }
    else
    {
        FILE* f = fopen(path, "a+");
        if(f != NULL)
        {
            fprintf(f, "%u %s\n", (unsigned)time(NULL), str);
            fclose(f);
        }
    }
    free(str);
    return res;
}
#endif
