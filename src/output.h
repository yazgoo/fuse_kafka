#ifndef OUTPUT_H
#define OUTPUT_H
#ifndef TEST
#include <librdkafka/rdkafka.h>
void
#ifdef MINGW_VER
__declspec(dllexport)
#endif
    input_setup_internal(int argc, char** argv, void* conf);
/* beware: zookeeper.h undefs __declspec */
#include <zookeeper/zookeeper.h>
#include "config.h"
#endif
typedef int (*output_setup_t)(kafka_t* k, config* fk_conf);
typedef int (*output_send_t)(kafka_t* k, char* buf, size_t len);
typedef int (*output_clean_t)(kafka_t* k);
typedef int (*output_update_t)();
#endif
