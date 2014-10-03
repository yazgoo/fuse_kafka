/** @file 
 * @brief Utility to send trace from fuse_kafka into kafka 
 **/ 
/**
 * @brief formats and send traces to kafka, behaving like printf
 * @param fmt the format of the string to send as defined in printf
 * @return 0 on success
 **/ 
static int trace(const char* fmt, ...)
{
    char* str;
    int res;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&str, fmt, ap);
    va_end(ap);
    res = actual_kafka_write("./fuse_kafka.log", str, strlen(str), 0);
    free(str);
    return res;
}
