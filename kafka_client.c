#ifndef TEST
#include <librdkafka/rdkafka.h>
#else
#include "kafka_client_test.c"
#endif // TEST
typedef struct _kafka_t
{
    rd_kafka_t* rk;
    rd_kafka_topic_t* rkt;
    config* conf;
} kafka_t;
static void msg_delivered (rd_kafka_t *rk,
                         void *payload, size_t len,
                         int error_code,
                         void *opaque, void *msg_opaque) {

    /*printf("================== message delivered %s\n",
            (char*) payload);*/
}
static void logger (const rd_kafka_t *rk, int level,
                 const char *fac, const char *buf) {
        /*struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(stderr, "%u.%03u UGUU RDKAFKA-%i-%s: %s: %s\n",
                (int)tv.tv_sec, (int)(tv.tv_usec / 1000),
                level, fac, rd_kafka_name(rk), buf);*/
}
char errstr[512];
int setup_kafka(kafka_t* k)
{
    char* brokers = "localhost:9092";
    char* topic = "bloh";
    brokers = ((config*) fuse_get_context()->private_data)->brokers[0];
    topic = ((config*) fuse_get_context()->private_data)->topic[0];
    rd_kafka_topic_conf_t *topic_conf;
    rd_kafka_conf_t *conf;
    conf = rd_kafka_conf_new();
    topic_conf = rd_kafka_topic_conf_new();
    rd_kafka_conf_set_dr_cb(conf, msg_delivered);
    if(rd_kafka_conf_set(conf, "debug", "all",
                errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK || 
            rd_kafka_conf_set(conf, "batch.num.messages", "1",
                errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        printf("%% Debug configuration failed: %s: %s\n",
                errstr, "all");
        return(1);
    }
    if (!(k->rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf,
                    errstr, sizeof(errstr)))) {
        fprintf(stderr,
                "%% Failed to create new producer: %s\n",
                errstr);
        return(1);
    }
    rd_kafka_set_logger(k->rk, logger);
    rd_kafka_set_log_level(k->rk, 7);
    if (rd_kafka_brokers_add(k->rk, brokers) == 0) {
        fprintf(stderr, "%% No valid brokers specified\n");
        return(1);
    }
    k->rkt = rd_kafka_topic_new(k->rk, topic, topic_conf);
    if(k->rkt == NULL)
        printf("topic %s creation failed\n", topic);
    return k->rkt == NULL;
}
int send_kafka(kafka_t* k, char* buf, size_t len)
{
    int r = 0;
    if((r = rd_kafka_produce(k->rkt, RD_KAFKA_PARTITION_UA,
            RD_KAFKA_MSG_F_COPY,
            buf, len,
            NULL, 0, NULL)))
        printf("=========== rd_kafka_produce: failed %d\n", r);
    fprintf(stderr, "%% Sent %zd bytes to topic "
            "%s\n",
            len, rd_kafka_topic_name(k->rkt));
    /*if((r = rd_kafka_poll(k->rk, 10)) != 1)
        printf("============= rd_kafka_poll: failed %d\n", r);*/
    /*while(rd_kafka_poll(k->rk, 1000) != -1)
        continue;*/
    return 0;
}
