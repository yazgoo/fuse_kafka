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
