#ifndef OUTPUT_H
#define OUTPUT_H
#include <librdkafka/rdkafka.h>
#include <zookeeper/zookeeper.h>
#include "config.h"
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
} kafka_t;
#endif
