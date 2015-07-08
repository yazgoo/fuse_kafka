#ifndef TRACE_H
#define TRACE_H
#include <stdarg.h>
static int trace(const char* fmt, ...);
#define trace_warn trace
#define trace_error trace
#ifdef FK_DEBUG
#define trace_debug(...) trace(__VA_ARGS__)
#else
#define trace_debug(...) /* trace(__VA_ARGS__) */
#endif
#endif
