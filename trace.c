static int trace(const char* fmt, ...)
{
    char* str;
    int res;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&str, fmt, ap);
    va_end(ap);
    res = actual_kafka_write("./fuse_kafka.log", str, strlen(str), 0, NULL);
    free(str);
    return res;
}
