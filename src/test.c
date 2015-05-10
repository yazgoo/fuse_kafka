/** @file */ 
/* added after modular_input */
#include <sys/types.h>
#include <dirent.h>
#include "fuse.h"
#include "kafka_client.c"
#include "output.c"
/* end added after modular_input */
#define fuse_get_context(a) test_fuse_get_context(a)
#define STRINGIFY(x) #x
#include "minunit.h"
// LCOV_EXCL_START
#define SET_CONFIG \
    static char* directories[] = {"/lol/"};\
    static char* excluded_files[] = {"xd"};\
    config conf;\
    conf.directories = directories;\
    conf.directory_n = 0;\
    conf.excluded_files_n = 1;\
    conf.excluded_files = excluded_files;\
    conf.fields_s = "{}";\
    conf.tags_s = "";\
    conf.quota_queue = NULL;\
    conf.quota_n = 0;\
    struct fuse_context* context = fuse_get_context();\
    context->pid = getpid();\
    kafka_t private_data;\
    private_data.conf = &conf;\
    context->private_data = (void*) &private_data;
static char* get_file_content(char* path)
{
    struct stat st;
    char* content;
    FILE* f;
    size_t read_nb_item;
    mu_assert("failed stating path", !stat(path, &st));
    content = (char*) malloc(st.st_size + 1);
    mu_assert("failed opening path", f = fopen(path, "r"));
    read_nb_item = fread((void*) content, st.st_size, 1, f);
    printf("read item number: %lu, file size: %lu\n", read_nb_item, st.st_size);
    mu_assert("did not read all file", read_nb_item == 1);
    fclose(f);
    content[st.st_size] = 0;
    return content;
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
    private_data.topic = &topic;
    private_data.zookeepers_n = 0;
    private_data.quota_n = 1;
    private_data.quota = quota;
    k.rk = &rk;
    fuse_get_context()->private_data = (void*) &private_data;
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    mu_assert("setup_kafka failed", setup_kafka(&k) == 0);
    test_with()->rd_kafka_conf_set_returns = 0;
    mu_assert("setup_kafka succeeded", setup_kafka(&k) == 1);
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    test_with()->rd_kafka_new_returns_NULL = 1;
    mu_assert("setup_kafka with kafka "
            "new returning NULL succeeded", setup_kafka(&k) == 1);
    test_with()->rd_kafka_new_returns_NULL = 0;
    test_with()->rd_kafka_brokers_add_returns = 0;
    mu_assert("setup_kafka with kafka rokers add failing",
            setup_kafka(&k) == 1);
    test_with()->rd_kafka_brokers_add_returns = 1;
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("setup_kafka with kafka topic new failing",
            setup_kafka(&k) == 1);
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    test_with()->rd_kafka_conf_set_fails_for = "batch.num.messages";
    mu_assert("setup_kafka should fail here",
            setup_kafka(&k) == 1);
    test_with()->rd_kafka_conf_set_fails_for = NULL;
    /* TODO move to a specific unit test
    mu_assert("kafka_init failed", kafka_init(NULL));
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("kafka_init succeeded", !kafka_init(NULL));
    */
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    mu_assert("fuse kafka main error",
            !fuse_kafka_main(argc, argv));
    test_with()->rd_kafka_produce_returns = 1;
    mu_assert("send_kafka return something other than 0",
            !send_kafka(&k, NULL, 0));
    test_with()->rd_kafka_produce_returns = 0;
    return 0;
}
static char* test_parse_arguments()
{
    config conf;
    memset(&conf, 0, sizeof(config));
    char* argv[] = {"--topic", "logs", "--fields", "datacenter", "eu-west-1a", 
        "--directories", "/usr/local/tomcat1/logs", "--brokers", "server:9092",
        "--tags", "gen", "--persist", "no", "--excluded_files", "blah",
        "--substitutions", "lol"};
    int argc = sizeof(argv)/sizeof(char*);
    char* argv2[] = {"--lol"};
    char* argv3[] = {"-lol"};
    mu_assert("parse arguments failed", parse_arguments(argc, argv, &conf));
    mu_assert("parse arguments succeeded", !parse_arguments(1, argv2, &conf));
    mu_assert("parse arguments succeeded", parse_arguments(1, argv3, &conf));
    return 0;
}
static char* test_logging()
{
    char* message = "hello";
    msg_delivered(NULL, message, strlen(message), 0, NULL, NULL);
    logger(NULL, 0, NULL, NULL);
    return 0;
}
static char* test_utils()
{
    char* args[] = {"lol", "xd", "pdtr"};
    char* args2[] = {"xd", "--", "--lol"};
    char* container;
    printf("command line is %s\n", get_command_line(1));
    mu_assert("cmdline for process #1 should contain init or boot.sh or '/bin/bash /ds/build.sh install'",
            strstr(get_command_line(1), "aW5pd") != NULL
            || strstr(get_command_line(1), "Ym9vd") != NULL
            || strstr(get_command_line(1), "L2Jpbi9iYXNoIC9kcy9idWlsZC5zaCBpbnN0YWxsIA==") != NULL);
    mu_assert("found a process with UINT_MAX as pid!",
            !strcmp("", get_command_line(UINT_MAX)));
    mu_assert("getting limit failed", get_limit(2, args) == 2);
    container = array_to_container_string(args, 3, '[', ']', ',', ',');
    mu_assert("parsing argument should have failed",
            !fuse_kafka_main(3, args2));
    free(container);
    return 0;
}
static char* test_time_queue()
{
    time_queue* queue = time_queue_new(10, 42);
    time_queue_set(queue, "a");
    mu_assert("time queue item should be null",
            time_queue_get(queue, "") == NULL);
    mu_assert("time queue does not overflows",
            time_queue_overflows(queue, "a", 42) == 1);
    *(time_queue_get(queue, "a")) -= 1000;
    mu_assert("time queue does not overflows",
            time_queue_overflows(queue, "a", 42) == 1);
    time_queue_set(queue, "a");
    time_queue_delete(queue);
    return 0;
}
int verbose_string_list_add(string_list** list, char* word)
{
    printf("adding %s on a list(%d, %d)\n",
            word, (*list)->size, (*list)->max_size);
    return string_list_add(list, word);
}
static char* test_string_list_fillup_to(
        char* word, string_list* list, size_t size)
{
    int i;
    for(i = 0; i < size; i++)
    {
        word[4] = '0' + i;
        mu_assert("server list add should work",
                !verbose_string_list_add(&list, word));
    }
    return 0;
}
static char* test_string_list()
{
    string_list* list = NULL, *list2 = NULL, *list3 = NULL;
    mu_assert("server list should not contain blah",
            !string_list_contains(&list, "blah"));
    mu_assert("server list add should work",    
            !string_list_add(&list, "blah"));
    mu_assert("server list should not contain foo",
            !string_list_contains(&list, "foo"));
    mu_assert("server list should contain blah",
            string_list_contains(&list, "blah"));
    char word[10];
    strcpy(word, "word_");
    test_string_list_fillup_to(word, list,
            SERVER_LIST_DEFAULT_MAX_SIZE);
    word[3] = 'm';
    test_string_list_fillup_to(word, list,
            SERVER_LIST_DEFAULT_MAX_SIZE - 1);
    *(falloc_fails()) = 1;
    word[1] = 'a';
    mu_assert("adding word should fail since the list resize should fail",
            verbose_string_list_add(&list, word));
    *(falloc_fails()) = 0;
    *(fcalloc_fails()) = 1;
    mu_assert("adding word should fail because of calloc fail",
            verbose_string_list_add(&list, word));
    mu_assert("list add once should fail",
            2 == string_list_add_once(&list, word));
    *(fcalloc_fails()) = 0;
    string_list_free(&list);
    *(fcalloc_fails()) = 1;
    mu_assert("creating a new list should fail because of calloc failure",
            string_list_new(&list2));
    *(fcalloc_fails()) = 0;
    string_list_free(&list2);
    *(falloc_fails()) = 1;
    mu_assert("creating a new list should fail because of malloc failure",
            string_list_add(&list3, word));
    *(falloc_fails()) = 0;
    string_list_free(&list3);
    return 0;
}
static char* test_server_list()
{
    server_list* list = NULL;
    char blah[] = "blah";
    char blah_foo[] = "blah,foo";
    char foo[] = "foo";
    char foo_blah[] = "foo,blah";
    char empty[] = "";
    mu_assert("adding blah once should succeed",
            server_list_add_once(&list, blah));
    mu_assert("adding blah once should fail since it was already added",
            !server_list_add_once(&list, blah));
    mu_assert("adding blah once should succed: foo was not registered",
            server_list_add_once(&list, blah_foo));
    mu_assert("adding blah once should not succed: foo was registered",
            !server_list_add_once(&list, foo));
    mu_assert("adding blah once should not succed: foo and blah registered",
            !server_list_add_once(&list, foo_blah));
    mu_assert("adding empty once should succeed",
            server_list_add_once(&list, empty));
    mu_assert("adding NULL once should fail",
            !server_list_add_once(&list, NULL));
    server_list_free(&list);
    return 0;
}
static char* test_zookeeper()
{
    char* topics[] = {"test"};
    char* brokers[] = {0};
    rd_kafka_t rk;
    kafka_t k;
    config conf;
    k.conf = &conf;
    k.conf->topic = topics;
    k.conf->brokers = brokers;
    k.rk = &rk;
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    mu_assert("zhandle_t should not be null",
            initialize_zookeeper("", &k) != NULL);
    mu_assert("zhandle_t should be null",
            initialize_zookeeper(NULL, &k) == NULL);
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("zhandle_t should not be null",
            initialize_zookeeper("", &k) != NULL);
    test_with()->zoo_get_children_returns = 0;
    mu_assert("zhandle_t should not be null",
            initialize_zookeeper("", &k) != NULL);
    /* for coverage */
    rd_kafka_destroy(NULL);
    rd_kafka_wait_destroyed(42);
    rd_kafka_topic_destroy(NULL);
    zookeeper_close(NULL);
    return 0;
}
static char* test_trace()
{
    SET_CONFIG;
    trace("blah");
    return 0;
}
void touch(char* path)
{
    FILE* f = fopen(path, "w");
    flock(fileno(f), LOCK_EX);
    fwrite("blah", 1, 1, f);
    fclose(f);
}
static char* test_dynamic_configuration()
{
    char* line;
    char** argv;
    int argc;
    char* conf_path = "/tmp/fuse_kafka_test_dynamic_configuration";
    mu_assert("loading dynamic configuration should fail",
            dynamic_configuration_load() == 1);
    touch(conf_path);
    dynamic_configuration_get()->path = conf_path;
    mu_assert("parse_line_from_file should return 1",
            parse_line_from_file(NULL, NULL, NULL) == 1);
    mu_assert("parse_args_from_file should return 1",
            parse_args_from_file(NULL, &argc, &argv, &line) == 1);
    dynamic_configuration_free();
    dynamic_configuration_load();
    mu_assert("dynamic_configuration_changed should return 0",
            dynamic_configuration_changed() == 0);
    //touch(conf_path);
    unlink(conf_path);
    dynamic_configuration_watch_stop();
    return 0;
}
static char* all_tests()
{
    // TODO move to specific unit test mu_run_test(test_setup_kafka);
    mu_run_test(test_parse_arguments);
    mu_run_test(test_logging);
    mu_run_test(test_utils);
    mu_run_test(test_time_queue);
    mu_run_test(test_zookeeper);
    // TODO move to specific unit test mu_run_test(test_trace);
    mu_run_test(test_string_list);
    mu_run_test(test_server_list);
    mu_run_test(test_dynamic_configuration);
    return 0;
}
// LCOV_EXCL_STOP because we don't want coverage on unit tests
#include "minunit.c"
