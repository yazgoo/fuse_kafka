/** @file */ 
/* added after modular_input */
#include <sys/types.h>
#include <dirent.h>
#include "fuse.h"
#include "hash.c"
#include "kafka_client.c"
#include "test_config.c"
#include "output.c"
/* end added after modular_input */
#define STRINGIFY(x) #x
#include "minunit.h"
// LCOV_EXCL_START
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
static char* test_utils()
{
    char* args[] = {"lol", "xd", "pdtr"};
    char* args2[] = {"xd", "--", "--lol"};
    char* args3[] = {"xd", "--", "--topic", "test"};
    char* container;
    *get_command_line_size() = 1;
    printf("command line is %s\n", get_command_line(1));
    *get_command_line_size() = 256;
    mu_assert("cmdline for process #1 should contain init or boot.sh or "
            "'/bin/bash /ds/build.sh install' or bash (docker)",
            strstr(get_command_line(1), "aW5pd") != NULL
            || strstr(get_command_line(1), "Ym9vd") != NULL
            || strstr(get_command_line(1), "L2Jpbi9iYXNoIA" /*docker*/) != NULL
            || strstr(get_command_line(1), "L2Jpbi9iYXNoIC9kcy9idWlsZC5zaCBpbnN0YWxsIA==") != NULL);
    mu_assert("found a process with UINT_MAX as pid!",
            !strcmp("", get_command_line(UINT_MAX)));
    mu_assert("getting limit failed", get_limit(2, args) == 2);
    container = array_to_container_string(args, 3, '[', ']', ',', ',');
    mu_assert("parsing argument should have failed",
            !fuse_kafka_main(3, args2));
    mu_assert("parsing argument should have succeed",
            fuse_kafka_main(4, args3) == 0);
    free(container);
    char* result = concat(args[0], args[1]);
    mu_assert("concatenation result should not be null",
            result != NULL);
    printf("result is %s\n", result);
    mu_assert("concatenation should be lol/xd",
            strcmp("lol/xd", result) == 0);
    free(result);
    char timestamp[] = "YYYY-MM-ddTHH:mm:ss.SSS+0000";
    set_timestamp(timestamp);
    mu_assert("timestamp should not be empty",
            timestamp[0] != 0);
    char* dir = "/tmp/mylittledir/";
    rmdir(dir);
    mkdir_p(dir);
    DIR* d = opendir(dir);
    mu_assert("timestamp should not be empty", d != NULL);
    closedir(d);
    return 0;
}
static int expect_base64(char* input, char* expected)
{
    char* output = base64(input, strlen(input));
    int result = (strcmp(expected, output) == 0);
    printf("base64 output: %s\n", output);
    free(output);
    return result;
}
static char* test_utils_base64()
{
#define b64_assert(a, b) {\
    mu_assert("base64 encoding \"" a "\" should return \"" b "\"", \
            expect_base64(a, b)); }
    mu_assert("base64(NULL) should return NULL", base64(NULL, 0) == NULL);
    b64_assert("", "")
    b64_assert("0", "MA==")
    b64_assert("1", "MQ==")
    b64_assert("42", "NDI=")
    b64_assert("hello, world", "aGVsbG8sIHdvcmxk")
    b64_assert("sit amet, consectetur adipiscing elit. Aenean ut gravida.",
            "c2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4gQWVuZWFuIHV0IGdyYXZpZGEu")
    b64_assert("sit amet, consectetur adipiscing elit. Aenean ut gravida",
            "c2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4gQWVuZWFuIHV0IGdyYXZpZGE=")
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
    set_brokerlist_from_zookeeper(NULL, NULL);
    zhandle_t zzh;
    set_brokerlist_from_zookeeper(&zzh, NULL);
    test_with()->zoo_get_children_returns = 1;
    set_brokerlist_from_zookeeper(&zzh, NULL);
    char* b = (char*) malloc(20);
    set_brokerlist_from_zookeeper(&zzh, b);
#define test_output_expected "a:2181,a:2181"
    mu_assert("b should equal " test_output_expected, strcmp(test_output_expected, b) == 0);
    free(b);
    k.conf->topic_n = 1;
    watcher(NULL, 0, 0, 0, &k);
    k.conf->brokers = topics;
    watcher(NULL, 0, 0, 0, &k);
    watcher_add_brokers(&k, "brokers", "topic");
    return 0;
}
static char* test_trace()
{
    SET_CONFIG;
    trace("blah");
    return 0;
}
void dynamic_configuration_handler(int argc, char**argv, void* context)
{
    *dynamic_configuration_watch_routine_running() = 0;
}
void zktouch(char* path)
{
    FILE* f = fopen(path, "w");
    char* str = "--zookeepers  test ";
    flock(fileno(f), LOCK_EX);
    fwrite(str, strlen(str), 1, f);
    fclose(f);
}
static char* test_dynamic_configuration()
{
    char* line;
    char** argv;
    int argc;
    char* conf_path = "/tmp/fuse_kafka_test_dynamic_configuration";
    unlink(conf_path);
    unlink("/tmp/fuse_kafka.args");
    mu_assert("loading dynamic configuration should fail",
            dynamic_configuration_load() == 1);
    zktouch(conf_path);
    dynamic_configuration_get()->path = conf_path;
    mu_assert("parse_line_from_file should return 1",
            parse_line_from_file(NULL, NULL, NULL) == 1);
    mu_assert("parse_args_from_file should return 1",
            parse_args_from_file(NULL, &argc, &argv, &line) == 1);
    mu_assert("parse_args_from_file should return 0",
            parse_args_from_file(conf_path, &argc, &argv, &line) == 0);
    dynamic_configuration_free();
    dynamic_configuration_load();
    mu_assert("dynamic_configuration_changed should return 1",
            dynamic_configuration_changed() == 1); /* TODO should be 0 */
    dynamic_configuration_watch_stop();
    *(dynamic_configuration_get_last_change()) = 1;
    dynamic_configuration_get()->context = (void*) 1;
    dynamic_configuration_watch_routine(dynamic_configuration_handler);
    mu_assert("dynamic configuration watch routine should have been fired up",
            *dynamic_configuration_watch_routine_running() == 0);
    *dynamic_configuration_watch_routine_running() = 1;
    dynamic_configuration_get()->context = NULL;
    unlink(conf_path);
    return 0;
}
static char* test_fk_hash()
{
    fk_hash hash = fk_hash_new();
    fk_hash_put(hash, "test", (void*)42, 1);
    printf("test value: %p\n", fk_hash_get(hash, "test", 1));
    mu_assert("test should be 42", fk_hash_get(hash, "test", 1) == (void*)42);
    // teest hashes the same
    fk_hash_put(hash, "teest", (void*)43, 1);
    mu_assert("teest should be 43", fk_hash_get(hash, "teest", 1) == (void*)43);
    mu_assert("test #2 should be 42", fk_hash_get(hash, "test", 1) == (void*)42);
    fk_hash_put(hash, "test", (void*)40, 1);
    mu_assert("test #3 should be 40", fk_hash_get(hash, "test", 1) == (void*)40);
    fk_hash_remove(hash, "test", 1, 0, 0);
    mu_assert("test should be -1", fk_hash_get(hash, "test", 1) == (void*)-1);
    fk_hash_put(hash, "teeest", (void*)44, 1);
    mu_assert("teeest should be 44", fk_hash_get(hash, "teeest", 1) == (void*)44);
    fk_hash_remove(hash, "teeest", 1, 0, 0);
    mu_assert("teeest should be -1", fk_hash_get(hash, "teeest", 1) == (void*)-1);
    fk_hash_remove(hash, "test", 1, 0, 0);
    mu_assert("test should be -1", fk_hash_get(hash, "test", 1) == (void*)-1);
    fk_hash_remove(hash, "teest", 1, 0, 0);
    mu_assert("test should be -1", fk_hash_get(hash, "teest", 1) == (void*)-1);
    fk_hash_delete(hash, 0, 0);
    fk_hash_list_delete(fk_hash_list_new(0, 0), 0, 0);
    return 0;
}
static char* test_my_input_setup()
{
    conf.input_n = 1;
    char* argv[] = {"nonexisting"};
    conf.input = argv;
    mu_assert("test_my_input_setup should return 1", 
            my_input_setup(0, argv, 1) == 1);
    return 0;
}
static char* test_output()
{
    SET_CONFIG;
    char* excluded = "excluded";
    char* quota = "10000";
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    test_with()->rd_kafka_topic_new_returns_NULL = 0;
    test_with()->rd_kafka_conf_set_returns = 0;
    conf.brokers_n = conf.zookeepers_n = 0;
    conf.output_n = 0;
    void* output = output_init(NULL);
    mu_assert("output_init(NULL) should be null", output == NULL);
    output = output_init(&conf);
    mu_assert("output should be null", output != NULL);
    test_with()->rd_kafka_conf_set_returns = RD_KAFKA_CONF_OK;
    output = output_init(&conf);
    mu_assert("output should not be null", output != NULL);
    conf.quota_queue = time_queue_new(10, 42);
    conf.quota_n = 1;
    conf.quota = &quota;
    conf.topic_n = 0;
    output_destroy(output);
    conf.output = &quota;
    conf.output_n = 1;
    output = output_init(&conf);
    mu_assert("output should be null with wrong output plugin pecified", output == NULL);
    conf.output_n = 0;
    output = output_init(&conf);
    mu_assert("output is not null", output != NULL);
    /* TODO uncomment
    mu_assert("sending empty string succeeds",
            send_kafka(output, "", 0) == 0);*/
    output_write("", "", "", 0, 0);
    test_with()->asprintf_sets_NULL = 1;
    conf.excluded_files_n = 1;
    conf.excluded_files = &excluded;
    conf.quota_queue = time_queue_new(10, 42);
    conf.quota_n = 1;
    conf.quota = &quota;
    conf.topic_n = 0;
    conf.encoder_n = 0;
    ((kafka_t*) output)->rkt = (void*) 1;
    fuse_get_context()->private_data = output;
    mu_assert("should not write to kafka excluded file",
            should_write_to_kafka(excluded, 0) == 0);
    mu_assert("should write to kafka not excluded file",
            should_write_to_kafka("test", 0) == 1);
    mu_assert("actual_kafka_write should return 1 if asprintf is failing",
            actual_kafka_write("", "", "", 0, 0) == 1);
    test_with()->asprintf_sets_NULL = 0;
    output_write("", "", "", 0, 0);
    mu_assert("actual_kafka_write should return 0 if asprintf is not failing",
            actual_kafka_write("", "", "", 0, 0) == 0);
    conf.encoder_n = 1;
    char* encoder[] = {"text"};
    conf.encoder = encoder;
    mu_assert("actual_kafka_write should return 0 if asprintf is not failing",
            actual_kafka_write("", "", "", 0, 0) == 0);
    conf.encoder_n = 0;
    conf.zookeepers_n = conf.brokers_n = 0;
    input_setup_internal(0, NULL, &conf);
    ((kafka_t*)output)->zhandle = (void*) 1;
    setup_from_dynamic_configuration(0, NULL, output);
    char* argv[] = {"--zookeepers", "zk"};
    int argc = sizeof(argv)/sizeof(char*);
    setup_from_dynamic_configuration(argc, argv, output);
    output_destroy(output);
    return 0;
}
static char* test_plugin()
{
    void* f = load_plugin_function(OUTPUT_PLUGIN_PREFIX, "kafka", "blah");
    mu_assert("function should not be loaded", f == NULL);
    f = load_plugin_function(OUTPUT_PLUGIN_PREFIX, "kafka", "output_setup");
    mu_assert("function should be loaded", f != NULL);
    return 0;
}
int test_queue_n;
char test_queue_chars[] = {0, 0};
void test_queue_callback(const char *prefix, const char *path, char *buf,
        size_t size, off_t offset)
{
    test_queue_chars[test_queue_n++] = path[0];
}
static char* test_queue()
{
    events_dequeue(test_queue_callback);
    *(event_queue_max_size()) = 2;
    event_enqueue("a", "a", "a", 1, 0);
    event_enqueue("b", "b", "b", 1, 0);
    mu_assert("size should be 2", *(event_queue_size()) == 2);
    event_enqueue("c", "c", "c", 1, 0);
    mu_assert("size should still be 2", *(event_queue_size()) == 2);
    event_enqueue("d", "d", "d", 1, 0);
    mu_assert("size should again still be 2", *(event_queue_size()) == 2);
    test_queue_n = 0;
    events_dequeue(test_queue_callback);
    mu_assert("enqueued n should be 2", test_queue_n == 2);
    mu_assert("enqueued first char should be 'c'", test_queue_chars[0] == 'c');
    mu_assert("enqueued second char should be 'd'", test_queue_chars[1] == 'd');
    return 0;
}
static char* all_tests()
{
    *(fk_sleep_enabled()) = 0;
    mu_run_test(test_parse_arguments);
    mu_run_test(test_utils);
    mu_run_test(test_utils_base64);
    printf("a\n");
    mu_run_test(test_time_queue);
    printf("b\n");
    mu_run_test(test_output);
    mu_run_test(test_zookeeper);
    mu_run_test(test_trace);
    mu_run_test(test_string_list);
    mu_run_test(test_server_list);
    mu_run_test(test_dynamic_configuration);
    mu_run_test(test_fk_hash);
    mu_run_test(test_my_input_setup);
    mu_run_test(test_plugin);
    mu_run_test(test_queue);
    return 0;
}
// LCOV_EXCL_STOP because we don't want coverage on unit tests
#include "minunit.c"
