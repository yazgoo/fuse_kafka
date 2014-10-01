#define VERSION "0.1.3"
#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <fuse.h>
//#include <ulockmgr.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <pwd.h>
#include <fnmatch.h>
#include <stdarg.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <stdlib.h>
#include "time_queue.c"
extern char* PL_Base64Encode(const char* src, int srclen, char* dest);
#ifndef TEST
#include <librdkafka/rdkafka.h>
#else
#include <limits.h>
static struct fuse_context* test_fuse_get_context()
{
    static struct fuse_context ctx = { 0 };
    return &ctx;
}
typedef struct { } rd_kafka_t;
typedef struct { } rd_kafka_topic_t;
typedef struct { } rd_kafka_conf_t;
typedef struct { } rd_kafka_topic_conf_t;
typedef int rd_kafka_conf_res_t;
typedef int rd_kafka_type_t;
#define RD_KAFKA_CONF_OK 1
#define RD_KAFKA_PRODUCER 42
#define RD_KAFKA_PARTITION_UA 42
#define RD_KAFKA_MSG_F_COPY 42
rd_kafka_conf_t *rd_kafka_conf_new () { return 0; }
rd_kafka_topic_conf_t *rd_kafka_topic_conf_new () { return 0; }
void rd_kafka_conf_set_dr_cb (rd_kafka_conf_t *conf, void *f) { }
typedef struct
{
    int setup;
    rd_kafka_conf_res_t rd_kafka_conf_set_returns;
    char* rd_kafka_conf_set_fails_for;
    int rd_kafka_new_returns_NULL;
    int rd_kafka_topic_new_returns_NULL;
    int rd_kafka_brokers_add_returns;
    int rd_kafka_produce_returns;
    int asprintf_sets_NULL;
    int test_filler_returns;
} test_config;
static test_config* test_with()
{
    static test_config conf;
    if(!conf.setup)
    {
        conf.rd_kafka_brokers_add_returns = 1;
        conf.setup = 1;
    }
    return &conf;
}
#define asprintf(x, ...) (test_with()->asprintf_sets_NULL? (*x = NULL) == NULL : asprintf(x, __VA_ARGS__))
rd_kafka_conf_res_t rd_kafka_conf_set (rd_kafka_conf_t *conf,
				       const char *name,
				       const char *value,
				       char *errstr, size_t errstr_size)
{
    if(test_with()->rd_kafka_conf_set_fails_for == NULL)
            return test_with()->rd_kafka_conf_set_returns;
    else
        return strcmp(test_with()->rd_kafka_conf_set_fails_for,
                name) == 0?0:RD_KAFKA_CONF_OK;
}
rd_kafka_t *rd_kafka_new (rd_kafka_type_t type, rd_kafka_conf_t *conf,
			  char *errstr, size_t errstr_size)
{
    static rd_kafka_t rk;
    return test_with()->rd_kafka_new_returns_NULL ? NULL:&rk;
}
void rd_kafka_set_logger (rd_kafka_t *rk, void *f) { }
void rd_kafka_set_log_level (rd_kafka_t *rk, int level) { }
int rd_kafka_brokers_add (rd_kafka_t *rk, const char *brokerlist)
{ return test_with()->rd_kafka_brokers_add_returns; }
rd_kafka_topic_t *rd_kafka_topic_new (rd_kafka_t *rk, const char *topic,
				      rd_kafka_topic_conf_t *conf)
                                      {
                                          static rd_kafka_topic_t t;
                                          return test_with()->rd_kafka_topic_new_returns_NULL ? NULL:&t;
                                      }
const char *rd_kafka_topic_name (const rd_kafka_topic_t *rkt) { return 0; }
int rd_kafka_produce (rd_kafka_topic_t *rkt, int32_t partitition,
		      int msgflags,
		      void *payload, size_t len,
		      const void *key, size_t keylen,
		      void *msg_opaque)
{ return test_with()->rd_kafka_produce_returns; }
#endif // TEST
#include <grp.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define CONFIG_ITEM(name) char** name; size_t name ## _n;
typedef struct _config {
    int directory_fd;
    size_t directory_n;
    char* fields_s;
    char* tags_s;
    CONFIG_ITEM(directories)
    CONFIG_ITEM(persist)
    CONFIG_ITEM(excluded_files)
    CONFIG_ITEM(substitutions)
    CONFIG_ITEM(brokers)
    CONFIG_ITEM(topic)
    CONFIG_ITEM(fields)
    CONFIG_ITEM(tags)
} config;
#define XSTR(s) STR(s)
#define STR(s) #s
#define CONFIG_CURRENT(expected) if(!strcmp(name, STR(expected))) { current_size = &(conf->expected ## _n); conf->expected = argv + i + 1; }
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
void* kafka_init(struct fuse_conn_info *conn)
{
    config* conf = ((config*) fuse_get_context()->private_data);
    int directory_fd = conf->directory_fd;
    fchdir(directory_fd);
    close(directory_fd);
    kafka_t* k = (kafka_t*) malloc(sizeof(kafka_t));
    if(setup_kafka((kafka_t*) k))
    {
        printf("kafka_init: setup_kafka failed\n");
        return NULL;
    }
    k->conf = conf;
    return (void*) k;
}
static char* get_command_line(unsigned int pid)
{
    char* path, *out;
    char* command_line = NULL;
    size_t size = 0;
    size_t i;
    FILE* f = NULL;
    asprintf(&path, "/proc/%d/cmdline", pid);
    f = fopen(path, "r");
    free(path);
    if(f == NULL || getline(&command_line, &size, f) == -1)
    {
        out = (char*) malloc(1); out[0] = 0;
        return out;
    }
    else
    {
        printf("%d\n", (int) size);
        fclose(f);
        for(i = 0; i < size-1; i++)
            if(!command_line[i] && command_line[i+1]) command_line[i] = ' ';
        out = PL_Base64Encode(command_line, size, NULL);
        free(command_line);
        return out;
    }
}
void set_timestamp(char* timestamp)
{
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    strftime(timestamp, strlen(timestamp), "%Y-%m-%dT%H:%M:%S.000%z", &tm);
}
static int actual_kafka_write(const char *path, const char *buf,
        size_t size, off_t offset, struct fuse_file_info *fi)
{
    char* ret = NULL;
    (void) path;
    char timestamp[] = "YYYY-MM-ddTHH:mm:ss.SSS+0000";
    char* text = PL_Base64Encode(buf, size, NULL);
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
static int should_write_to_kafka(const char* path)
{
    kafka_t *private_data = (kafka_t*) fuse_get_context()->private_data;
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
    return 1;
}
static int kafka_write(const char *path, const char *buf,
        size_t size, off_t offset, struct fuse_file_info *fi)
{
    int res;
    if(should_write_to_kafka(path) &&
            actual_kafka_write(path, buf, size, offset, fi)) return 1;
    res = pwrite(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;

    return res;
}

#include "overlay.c"
int get_limit(int argc, char** argv)
{
    int i = 0;
    for(; i < argc; i++) if(!strcmp(argv[i], "--")) break;
    return i;
}
char* array_to_container_string(char** array, size_t n, char open_char,
        char close_char, char sep1, char sep2)
{
    int i = 0;
    char* str = (char*) malloc(3);
    int k = sprintf(str, "%c", open_char);
    for(i = 0; i < n; i++)
    {
        str = realloc(str, k + 1 + strlen(array[i]) + 2 + 2);
        k += sprintf(str + k, "\"%s\"", array[i]);
        if(i != n-1) k += sprintf(str + k, "%c ", i % 2 ? sep2 : sep1);
    }
    sprintf(str + k, "%c", close_char);
    return str;
}
void add_fields_and_tags(config* conf)
{
    conf->fields_s = array_to_container_string(
            conf->fields, conf->fields_n, '{', '}', ':', ',');
    conf->tags_s = array_to_container_string(
            conf->tags, conf->tags_n, '[', ']', ',', ',');
}
int parse_arguments(int argc, char** argv, config* conf)
{
    int i;
    size_t* current_size;
    char* name;
    for(i = 0; i < argc; i++)
    {
        if(strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-')
        {
            name = argv[i] + 2;
            CONFIG_CURRENT(directories)
            else CONFIG_CURRENT(persist)
            else CONFIG_CURRENT(excluded_files)
            else CONFIG_CURRENT(substitutions)
            else CONFIG_CURRENT(brokers)
            else CONFIG_CURRENT(topic)
            else CONFIG_CURRENT(fields)
            else CONFIG_CURRENT(tags)
            else
            {
                printf("unknown option %s\n", argv[i]);
                return 0;
            }
            *current_size = 0;
        }
        else (*current_size)++;
    }
    add_fields_and_tags(conf);
    return 1;
}
/*#include <signal.h>
void handler(int sig)
{
    printf("catched sigsegv\n");
    while (1) { };
}*/
int fuse_kafka_main(int argc, char *argv[])
{
    int i;
    int limit = get_limit(argc, argv);
    config conf;
    if(parse_arguments(argc - limit - 1, argv + limit + 1, &conf))
    {
        for(conf.directory_n = 0; conf.directory_n < conf.directories_n;
                conf.directory_n++)
        {
            argv[1] = conf.directories[conf.directory_n];
            if(!fork())
            {
                //signal(SIGSEGV, handler);
                conf.directory_fd = open(conf.directories[conf.directory_n],
                        O_RDONLY);
                for(i = 0;i < limit; i++) printf("fuse_main %s--\n", argv[i]);
                return fuse_main(limit, argv, &kafka_oper, &conf);
            }
        }
    }
    wait(NULL);
    return 0;
}
char* cmd = NULL;
#define RET_CMD(...) { asprintf(&cmd, __VA_ARGS__); system(cmd); free(cmd); }; return 0;
#ifdef TEST
#include "test.c"
#endif
int main(int argc, char** argv)
{
#ifndef TEST
    char* prefix = "__mountpoint__ -oallow_other -ononempty -s -omodules=subdir,subdir=. -- ";
    fuse_kafka_main(argc, argv);
#else
    char* result = all_tests();
    if (result != 0) printf("%s\n", result);
    else printf("ALL TESTS PASSED\n");
    printf("Tests run: %d\n", tests_run);
    return result != 0;
#endif
}
