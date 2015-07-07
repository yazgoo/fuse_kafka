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
#include <stdarg.h>
static int trace(const char* fmt, ...)
{
    char* str;
    int res;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&str, fmt, ap);
    va_end(ap);
    //res = actual_kafka_write("./fuse_kafka.log", str, strlen(str), 0);
    printf("%s\n", str);
    free(str);
    return res;
}
#define trace_warn trace
#define trace_error trace
#ifdef FK_DEBUG
#define trace_debug(...) trace(__VA_ARGS__)
#else
#define trace_debug(...) /* trace(__VA_ARGS__) */
#endif
#endif
