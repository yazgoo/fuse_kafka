/** @file */ 
#define fuse_get_context() test_fuse_get_context()
int tests_run = 0;
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
    if (message) return message; } while (0)
extern int tests_run;
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
static char* test_kafka_write()
{
    char* directories[] = {"/lol/"};
    char* cwd;
    char* content;
    const char* expected = "blah";
    char* file_path = "tmp_file";
    FILE* f;
    kafka_t private_data;
    struct fuse_file_info file_info;
    config conf;
    conf.directories = directories;
    conf.directory_n = 0;
    conf.fields_s = "{}";
    conf.tags_s = "";
    conf.quota_queue = NULL;
    conf.quota_n = 0;
    private_data.conf = &conf;
    struct fuse_context* context = fuse_get_context();
    context->pid = getpid();
    context->private_data = (void*) &private_data;
    cwd = get_current_dir_name();
    chdir(TEST);
    f = fopen(file_path, "w");
    mu_assert("f be opened", f != NULL);
    file_info.fh = fileno(f);
    kafka_write(file_path, expected, strlen(expected) + 1, 0, &file_info);
    fclose(f);
    content = get_file_content(file_path);
    printf("expected:%s\nactual:%s\n", expected, content);
    mu_assert("file content unexpected", !strcmp(expected, content));
    mu_assert("write succeeded while fd was closed!",
            kafka_write(file_path, expected, strlen(expected) + 1, 0, &file_info) < 0);
    test_with()->asprintf_sets_NULL = 1;
    fuse_get_context()->gid = UINT_MAX;
    fuse_get_context()->uid = UINT_MAX;
    mu_assert("write succeeded as printf setting NULL!",
            kafka_write(file_path, expected, strlen(expected) + 1, 0, &file_info) == 1);
    test_with()->asprintf_sets_NULL = 0;
    chdir(cwd);
    return 0;
}
int test_filler(void *buf,
        const char *name, const struct stat *stbuf, off_t off)
{
    return test_with()->test_filler_returns;
}
static char* test_passthrough_calls()
{
    struct stat st;
    struct fuse_file_info fi;
    struct timespec ts[2] = { { 0 } };
    struct statvfs stvfs;
    char* str = (char*) malloc(15);
#define TEST_FUNC(x, y, ...) mu_assert(#x " failed", x(__VA_ARGS__) == y);
#define TEST_FUNC_SUCCESS(x, y, ...) mu_assert(#x "(" #y ") failed", x(y, ##__VA_ARGS__) == 0);
#define TEST_FUNC_FAILURE(x, y, ...) mu_assert(#x "(" #y ") succeeded", x(y, ##__VA_ARGS__) != 0);
    TEST_FUNC_SUCCESS(kafka_getattr, "/", &st)
    TEST_FUNC_FAILURE(kafka_getattr, "/non-existing/path", &st)
    TEST_FUNC_FAILURE(kafka_fgetattr, "/", &st, &fi)
    fi.fh = open("/", O_DIRECTORY);
    TEST_FUNC_SUCCESS(kafka_fgetattr, "/", &st, &fi)
    close(fi.fh);
    fi.fh = -1;
    TEST_FUNC_SUCCESS(kafka_access, "/", 0)
    TEST_FUNC_FAILURE(kafka_access, "/non-existing/path", 0)
    TEST_FUNC_FAILURE(kafka_readlink, "/", 0, 0)
    TEST_FUNC_SUCCESS(kafka_readlink, "/proc/mounts", str, 15)
    TEST_FUNC_FAILURE(kafka_opendir, "/non-existing/path", &fi)
    TEST_FUNC_SUCCESS(kafka_opendir, "/", &fi)
    TEST_FUNC_SUCCESS(kafka_releasedir, "/", &fi)
    TEST_FUNC_SUCCESS(kafka_mknod, TEST "/node", S_IFREG, 0)
    TEST_FUNC_FAILURE(kafka_mknod, "/", S_IFREG, 0)
    TEST_FUNC_SUCCESS(kafka_mknod, TEST "/fifo", S_IFIFO, 0)
    TEST_FUNC_SUCCESS(kafka_mkdir, TEST "/dir", 0)
    TEST_FUNC_FAILURE(kafka_mkdir, TEST "/non-existing/dir", 0)
    TEST_FUNC_SUCCESS(kafka_rmdir, TEST "/dir")
    TEST_FUNC_FAILURE(kafka_rmdir, TEST "/dir")
    TEST_FUNC_SUCCESS(kafka_symlink, TEST "/from", TEST "/to")
    TEST_FUNC_FAILURE(kafka_symlink, TEST, TEST)
    TEST_FUNC_FAILURE(kafka_rename, TEST "/from", TEST "/to")
    TEST_FUNC_SUCCESS(kafka_rename, TEST "/node", TEST "/renamed")
    TEST_FUNC_FAILURE(kafka_link, TEST "/from", TEST "/to")
    TEST_FUNC_SUCCESS(kafka_link, TEST "/renamed", TEST "/lol")
    TEST_FUNC_FAILURE(kafka_chmod, TEST "/from", 0)
    TEST_FUNC_SUCCESS(kafka_chmod, TEST "/renamed", S_IWUSR | S_IRUSR)
    TEST_FUNC_SUCCESS(kafka_chown, TEST "/renamed", getuid(), getgid())
    TEST_FUNC_FAILURE(kafka_truncate, TEST "/from", 0)
    TEST_FUNC_SUCCESS(kafka_truncate, TEST "/renamed", 1)
    TEST_FUNC_FAILURE(kafka_ftruncate, TEST "/from", 0, &fi)
    fi.fh = open(TEST "/renamed", O_RDWR);
    TEST_FUNC_SUCCESS(kafka_ftruncate, TEST "/renamed", 1, &fi)
    close(fi.fh);
    TEST_FUNC_SUCCESS(kafka_unlink, TEST "/renamed")
    TEST_FUNC_FAILURE(kafka_unlink, TEST "/non-existing/file")
    TEST_FUNC_FAILURE(kafka_chown, TEST "/from", 0, 0)
    TEST_FUNC_FAILURE(kafka_utimens, TEST "/from", ts)
    TEST_FUNC_FAILURE(kafka_create, TEST "/from", 0, &fi)
    fi.flags = O_CREAT;
    TEST_FUNC_SUCCESS(kafka_create, TEST "/node", S_IWUSR |S_IRUSR, &fi)
    fi.flags = 0;
    TEST_FUNC_SUCCESS(kafka_open, TEST "/node", &fi)
    TEST_FUNC_SUCCESS(kafka_flush, TEST "/node", &fi)
    TEST_FUNC_SUCCESS(kafka_fsync, TEST "/node", 0, &fi)
    TEST_FUNC_SUCCESS(kafka_utimens, TEST "/node", ts)
    TEST_FUNC_SUCCESS(kafka_read, TEST "/node", str, 15, 0, &fi)
    close(fi.fh);
    TEST_FUNC_SUCCESS(kafka_statfs, TEST "/node", &stvfs)
    TEST_FUNC_FAILURE(kafka_open, TEST "/from", &fi)
    TEST_FUNC_FAILURE(kafka_read, TEST "/from", NULL, 0, 0, &fi)
    TEST_FUNC_FAILURE(kafka_statfs, TEST "/from", &stvfs)
    TEST_FUNC_FAILURE(kafka_flush, NULL, &fi)
    TEST_FUNC_SUCCESS(kafka_release, NULL, &fi)
    TEST_FUNC_FAILURE(kafka_fsync, NULL, 0, &fi)
    TEST_FUNC_SUCCESS(kafka_lock, NULL, &fi, 0, NULL)
    fi.fh = (uint64_t) opendir("/");
    TEST_FUNC_SUCCESS(kafka_readdir, "/", NULL, test_filler, 0, &fi)
    test_with()->test_filler_returns = 1;
    TEST_FUNC_SUCCESS(kafka_readdir, "/", NULL, test_filler, 0, &fi)
    test_with()->test_filler_returns = 0;
    closedir((DIR*) fi.fh);
    free(str);
    return 0;
}
static char* test_setup_kafka()
{
    rd_kafka_t rk;
    kafka_t k;
    config private_data;
    char* brokers[1] = {""};
    char* topic = "";
    char* argv[] = {"__mountpoint__", "--", "--directories",
        TEST "a", "--fields", "a", "b", "--tags", "1"};
    int argc = sizeof(argv)/sizeof(char*);
    private_data.brokers = brokers;
    private_data.topic = &topic;
    private_data.quota_n = 0;
    private_data.zookeepers_n = 0;
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
    mu_assert("kafka_init failed", kafka_init(NULL));
    test_with()->rd_kafka_topic_new_returns_NULL = 1;
    mu_assert("kafka_init succeeded", !kafka_init(NULL));
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
    mu_assert("first process should be init",
            !strcmp("L3NiaW4vaW5pdCA", get_command_line(1)));
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
    mu_assert("zhandle_t should not be null",
            initialize_zookeeper("", &k) != NULL);
    return 0;
}
static char* test_trace()
{
    trace("blah");
    return 0;
}
static char* all_tests()
{
    //mu_run_test(test_kafka_write);
    mu_run_test(test_passthrough_calls);
    mu_run_test(test_setup_kafka);
    mu_run_test(test_parse_arguments);
    mu_run_test(test_logging);
    mu_run_test(test_utils);
    mu_run_test(test_time_queue);
    mu_run_test(test_zookeeper);
    mu_run_test(test_trace);
    return 0;
}
// LCOV_EXCL_STOP because we don't want coverage on unit tests
int main(int argc, char** argv)
{
    char* result = all_tests();
    if (result != 0) printf("ASSERTION FAILED:\n%s\n", result);
    else printf("ALL TESTS PASSED\n");
    printf("Tests run: %d\n", tests_run);
    system("rm -f " TEST "/to");
    return result != 0;
}
