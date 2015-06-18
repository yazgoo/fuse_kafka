// LCOV_EXCL_START
#include "minunit.h"
#define _GNU_SOURCE // for get_current_dir_name
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "overlay.c"
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
    context->pid = getpid();
typedef struct
{
    int setup;
    char* rd_kafka_conf_set_fails_for;
    int rd_kafka_new_returns_NULL;
    int rd_kafka_topic_new_returns_NULL;
    int rd_kafka_brokers_add_returns;
    int rd_kafka_produce_returns;
    int zoo_get_children_returns;
    int asprintf_sets_NULL;
    int test_filler_returns;
}
test_config;
static test_config* test_with()
{
    static test_config conf;
    return &conf;
}
int test_filler(void *buf,
        const char *name, const struct stat *stbuf, off_t off)
{
    return test_with()->test_filler_returns;
}
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
    FILE* f;
    const char* expected = "blah";
    struct fuse_file_info file_info;
    char* content;
    char* file_path = "tmp_file";
    SET_CONFIG;
    char* cwd;
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
            kafka_write(file_path, expected, strlen(expected) + 1, 0, &file_info) <= 0);
    mu_assert("write succeeded as printf setting NULL!",
            kafka_write(excluded_files[0], expected, strlen(expected) + 1, 0, &file_info) <= 0);
    test_with()->asprintf_sets_NULL = 0;
    chdir(cwd);
    /* TODO uncomment
    conf.quota_queue = time_queue_new(10, 42);
    kafka_write(file_path, expected, strlen(expected) + 1, 0, &file_info);
    time_queue_delete(conf.quota_queue);
    */
    return 0;
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
    unlink(TEST "/node");
    TEST_FUNC_SUCCESS(kafka_mknod, TEST "/node", S_IFREG, 0)
    TEST_FUNC_FAILURE(kafka_mknod, "/", S_IFREG, 0)
    unlink(TEST "/fifo");
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
    unlink(TEST "/lol");
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
    TEST_FUNC_FAILURE(kafka_create, TEST "/var", 0, &fi)
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
    return NULL;
}
static char* test_other_functions()
{
    char* argv[] = {}; 
    kafka_init(NULL);
    // TODO uncomment mu_assert("input_setup should return -1", input_setup(0, NULL, NULL) == -1);
    return 0;
}
static char* all_tests()
{
    mu_run_test(test_passthrough_calls);
    mu_run_test(test_kafka_write);
    mu_run_test(test_other_functions);
    return 0;
}
#include "minunit.c"
// LCOV_EXCL_STOP
