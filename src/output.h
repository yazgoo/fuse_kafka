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
#include "server_list.h"
/**
 * @brief a wrapping structure for kafka client and fuse_kafka
 * configuration
 **/
typedef struct _kafka_t
{
    rd_kafka_t* rk;
    rd_kafka_topic_t* rkt;
    config* conf;
    /* were brokers given added to kafka (in zk mode): */
    char no_brokers;
    zhandle_t* zhandle;
    server_list* broker_list;
} kafka_t;
#endif
