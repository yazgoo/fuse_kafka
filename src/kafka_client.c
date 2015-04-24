/** @file
 * @brief kafka client level structures and function
 **/ 
#ifndef TEST
#include <librdkafka/rdkafka.h>
#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper.jute.h>
#else
#include "kafka_client_test.c"
#endif // TEST
#include "output.h"
#include "zookeeper.c"
/**
 * @brief send a string to kafka
 * @param k configuration with kafka
 * @param buf string to serialize
 * @param len size of the string to save
 **/
int send_kafka(kafka_t* k, char* buf, size_t len)
{
    int r = 0;
    if((r = rd_kafka_produce(k->rkt, RD_KAFKA_PARTITION_UA,
            RD_KAFKA_MSG_F_COPY,
            buf, len,
            NULL, 0, NULL)))
        printf("=========== rd_kafka_produce: failed %d\n", r);
    /*fprintf(stderr, "%% Sent %zd bytes to topic "
            "%s\n",
            len, rd_kafka_topic_name(k->rkt));*/
    /*if((r = rd_kafka_poll(k->rk, 10)) != 1)
        printf("============= rd_kafka_poll: failed %d\n", r);*/
    /*while(rd_kafka_poll(k->rk, 1000) != -1)
        continue;*/
    return 0;
}
