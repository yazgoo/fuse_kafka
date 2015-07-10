// LCOV_EXCL_START
#include "minunit.h"
#include "context.c"
#include "kafka_client_test.c"
#include "config.h"
#include "kafka.c"
int setup_kafka(kafka_t* k, config* fk_conf)
{
    return output_setup(k, fk_conf);
}
int send_kafka(kafka_t* k, char* buf, size_t len)
{
    return output_send(k, buf, len);
}
static char* test_setup_kafka()
{
    rd_kafka_t rk;
    kafka_t k;
    config private_data;
    char* brokers[1] = {""};
    char* quota[1] = {"1000000"};
    char* topic = "";
    char* argv[] = {"__mountpoint__", "--", "--directories",
        TEST "a", "--fields", "a", "b", "--tags", "1"};
    int argc = sizeof(argv)/sizeof(char*);
    private_data.brokers = brokers;
    private_data.brokers_n = 1;
    private_data.topic = &topic;
    private_data.zookeepers_n = 0;
    private_data.quota_n = 1;
    private_data.quota = quota;
    k.rk = &rk;
    fuse_get_context()->private_data = (void*) &private_data;
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    mu_assert("setup_kafka failed", setup_kafka(&k, &private_data) == 0);
    private_data.topic_n = 1;
    private_data.topic = brokers;
    private_data.zookeepers = brokers;
    private_data.zookeepers_n = 1;
    k.conf = NULL;
    mu_assert("setup_kafka with zookeepers should succeed", setup_kafka(&k, &private_data) == 0);
    private_data.zookeepers_n = 0;
    test_with()->rd_kafka_conf_set_returns = 0;
    mu_assert("setup_kafka succeeded", setup_kafka(&k, &private_data) == 1);
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    test_with()->rd_kafka_new_returns_NULL = 1;
    private_data.zookeepers = NULL;
    mu_assert("setup_kafka with kafka "
            "new returning NULL succeeded", setup_kafka(&k, &private_data) == 1);
    private_data.zookeepers = brokers;
    test_with()->rd_kafka_new_returns_NULL = 0;
    test_with()->rd_kafka_brokers_add_returns = 0;
    mu_assert("setup_kafka with kafka rokers add failing",
            setup_kafka(&k, &private_data) == 1);
    test_with()->rd_kafka_brokers_add_returns = 1;
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("setup_kafka with kafka topic new failing",
            setup_kafka(&k, &private_data) == 1);
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    test_with()->rd_kafka_conf_set_fails_for = "batch.num.messages";
    mu_assert("setup_kafka should fail here",
            setup_kafka(&k, &private_data) == 1);
    test_with()->rd_kafka_conf_set_fails_for = NULL;
    /* TODO move to a specific unit test
    mu_assert("kafka_init failed", kafka_init(NULL));
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("kafka_init succeeded", !kafka_init(NULL));
    */
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    test_with()->rd_kafka_produce_returns = 1;
    mu_assert("send_kafka return something other than 0",
            !send_kafka(&k, NULL, 0));
    k.rkt = (void*) 2;
    mu_assert("send_kafka return something other than 0",
            !send_kafka(&k, NULL, 0));
    test_with()->rd_kafka_produce_returns = 0;
    private_data.zookeepers = private_data.brokers = NULL;
    private_data.zookeepers_n = private_data.brokers_n = 0;
    mu_assert("setup_kafka with zookeepers should succeed", setup_kafka(&k, &private_data) == 0);
    return 0;
}
static char* test_logging()
{
    char* message = "hello";
    msg_delivered(NULL, message, strlen(message), 0, NULL, NULL);
    logger(NULL, 0, NULL, NULL);
    return 0;
}
static char* test_output_update_clean()
{
    char* zk[] = {""};
    output_update(NULL);
    output_clean(NULL);
    kafka_t k;
    memset(&k, 0, sizeof(kafka_t));
    output_clean(&k);
    output_update(&k);
    config c;
    c.zookeepers_n = 1;
    c.zookeepers = zk;
    k.conf = &c;
    output_update(&k);
    return 0;
}
static char* all_tests()
{
    mu_run_test(test_setup_kafka);
    mu_run_test(test_logging);
    mu_run_test(test_output_update_clean);
    return 0;
}
#include "minunit.c"
// LCOV_EXCL_STOP
