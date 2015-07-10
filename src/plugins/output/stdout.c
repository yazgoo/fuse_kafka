#include <output.h>
#include "trace.c"
int output_setup(kafka_t* k, config* fk_conf)
{
    trace_debug("stdout output_setup: setting rkt to 1\n");
    k->rkt = (void*) 1;
    return 0;
}
int output_send(kafka_t* k, char* buf, size_t len)
{
    trace("%s", buf);
    return 0;
}
