#include "version.h"
#include "output.h"
#include "time_queue.c"
#include "zookeeper.c"
#ifndef TEST
#include "dynamic_configuration.c"
#endif
#include <grp.h>
#include <pwd.h>
/**
 * @brief actually does the write to kafka of a string with the given
 * file path
 * @param path file path to save to kafka
 * @param buf write buffer
 * @param size size of the buffer to write
 * @param offset starting point in the buffer
 * @return 0 if the write succeeded, 1 otherwise
 **/
static int actual_kafka_write(const char *path, const char *buf,
        size_t size, off_t offset)
{
    char* ret = NULL;
    (void) path;
    char timestamp[] = "YYYY-MM-ddTHH:mm:ss.SSS+0000";
    char* text = base64(buf, size);
    struct fuse_context* context = fuse_get_context();
    struct group* sgroup = getgrgid(context->gid);
    struct passwd* suser = getpwuid(context->uid);
    char* user = suser == NULL ? "":suser->pw_name;
    char* group = sgroup == NULL ? "":sgroup->gr_name;
    char* command = get_command_line(context->pid);
    char* format = "{\"path\": \"%s%s\", \"pid\": %d, \"uid\": %d, "
        "\"gid\": %d, \"@message\": \"%s\", \"@timestamp\": \"%s\","
        "\"user\": \"%s\", \"group\": \"%s\", \"command\": \"%s\","
        "\"@version\": \"%s\", \"@fields\": %s, \"@tags\": %s}";
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    config* conf = (config*)private_data->conf;
    set_timestamp(timestamp);
    asprintf(&ret, format, conf->directories[conf->directory_n],
            path + 1, context->pid, context->uid, context->gid,
            text, timestamp, user, group, command, VERSION,
            conf->fields_s, conf->tags_s);
    free(command);
    free(text);
    if (ret == NULL) {
        fprintf(stderr, "Error in asprintf\n");
        return 1;
    }
    send_kafka(context->private_data, ret, strlen(ret));
    free(ret);
    return 0;
}
#include "trace.c"
/**
 * @brief checks if writes from the given path should be written to
 * kafka
 * @param path the write path
 * @param size the write size
 **/
static int should_write_to_kafka(const char* path, size_t size)
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
    if(private_data == NULL || private_data->rkt == NULL) return 0;
    config* conf = (config*)private_data->conf;
    int i = 0;
    for(i = 0; i < conf->excluded_files_n; i++)
    {
        char* pattern = conf->excluded_files[i];
        if(!fnmatch(pattern, path, 0))
        {
            return 0;
        }
    }
    if(conf->quota_queue == NULL) return 1;
    if(time_queue_overflows(conf->quota_queue, (char*)path, size)) i = 0;
    else i = 1;
    time_queue_set(conf->quota_queue, (char*)path);
    return i;
}
void output_write(const char *path, const char *buf,
        size_t size, off_t offset)
{
    if(should_write_to_kafka(path, size))
        actual_kafka_write(path, buf, size, offset);
}
void output_destroy(void* untyped)
{
    kafka_t* k = (kafka_t*) untyped;
    if(k == NULL) return;
    if(k->conf->quota_n > 0) time_queue_delete(k->conf->quota_queue);
    if(k->zhandle != NULL) zookeeper_close(k->zhandle);
    if(k->rkt != NULL) rd_kafka_topic_destroy(k->rkt);
    rd_kafka_destroy(k->rk);
    rd_kafka_wait_destroyed(1000);
    free(k);
    dynamic_configuration_watch_stop();
}
void setup_from_dynamic_configuration(int argc, char** argv, void* context)
{
    kafka_t* k = (kafka_t*) context;
    memset(k->conf, 0, sizeof(config));
    parse_arguments(argc, argv, k->conf);
    if(k->zhandle != NULL)
    {
        zookeeper_close(k->zhandle);
        k->zhandle = NULL;
    }
    if(k->conf->zookeepers_n > 0)
        k->zhandle = initialize_zookeeper(k->conf->zookeepers[0], k);
}
/**
 * @brief a librdkafka callback called when a message is delivered
 **/
static void msg_delivered (rd_kafka_t *rk,
                         void *payload, size_t len,
                         int error_code,
                         void *opaque, void *msg_opaque) {

    /*printf("================== message delivered %s\n",
            (char*) payload);*/
}
/**
 * @brief a librdkafka callback called to log stuff from librdkafka
 **/
static void logger (const rd_kafka_t *rk, int level,
                 const char *fac, const char *buf) {
        /*struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(stderr, "%u.%03u UGUU RDKAFKA-%i-%s: %s: %s\n",
                (int)tv.tv_sec, (int)(tv.tv_usec / 1000),
                level, fac, rd_kafka_name(rk), buf);*/
}
char errstr[512];
/**
 * @brief setup_kafka initialises librdkafka based on the config
 * wrapped in kafka_t
 * @param k kafka configuration
 **/
int setup_kafka(kafka_t* k)
{
    char* brokers = NULL;
    char* zookeepers = NULL;
    char* topic = "logs";
    config* fk_conf = (config*) fuse_get_context()->private_data;
    if(fk_conf->zookeepers_n > 0) zookeepers = fk_conf->zookeepers[0];
    if(fk_conf->brokers_n > 0) brokers = fk_conf->brokers[0];
    if(fk_conf->topic_n > 0) topic = fk_conf->topic[0];
    rd_kafka_topic_conf_t *topic_conf;
    rd_kafka_conf_t *conf;
    conf = rd_kafka_conf_new();
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
    if (zookeepers != NULL)
    {
        k->zhandle = initialize_zookeeper(zookeepers, k);
        return 0;
    }
    else if(brokers != NULL)
    {
        if (rd_kafka_brokers_add(k->rk, brokers) == 0) {
            fprintf(stderr, "%% No valid brokers specified\n");
            return(1);
        }
        topic_conf = rd_kafka_topic_conf_new();
        k->rkt = rd_kafka_topic_new(k->rk, topic, topic_conf);
        if(k->rkt == NULL)
            printf("topic %s creation failed\n", topic);
        return k->rkt == NULL;
    }
    return 0;
}
void* output_init(config* conf)
{
    dynamic_configuration_watch(&setup_from_dynamic_configuration);
    int directory_fd = conf->directory_fd;
    int time_queue_size;
    fchdir(directory_fd);
    close(directory_fd);
    kafka_t* k = (kafka_t*) malloc(sizeof(kafka_t));
    memset(k, 0, sizeof(kafka_t));
    if(setup_kafka((kafka_t*) k))
    {
        printf("kafka_init: setup_kafka failed\n");
        return NULL;
    }
    k->conf = conf;
    if(conf->quota_n > 0)
    {
        time_queue_size = conf->quota_n > 1 ? atoi(conf->quota[1]):20;
        conf->quota_queue = time_queue_new(
                time_queue_size, atoi(conf->quota[0]));
    }
    dynamic_configuration_get()->context = (void*) k;
    return (void*) k;
}
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
input_setup_internal(int argc, char** argv, void* conf)
{
    fuse_get_context()->private_data = conf;
    fuse_get_context()->private_data = output_init((config*) conf);
#ifndef TEST
    input_setup(argc, argv, conf);
#endif
}
