// LCOV_EXCL_START
#include "minunit.h"
#define INOTIFY_INIT_PATH "/tmp/inotify_init_test"
int inotify_init_test()
{
    touch(INOTIFY_INIT_PATH, "blah");
    return open(INOTIFY_INIT_PATH);
}
#define inotify_init inotify_init_test
#include "inotify.c"
static char* test_input_setup()
{
    char* argv[] = {"blah", "/tmp"}; 
    int n = sizeof(argv)/sizeof(char*);
    mu_assert("input_setup should return -1", input_setup(0, NULL, NULL) == -1);
    *(inotify_runnning()) = 0;
    mu_assert("input_setup should return 0", input_setup(n, argv, NULL) == 0);
    *(inotify_runnning()) = 1;
    mu_assert("input_setup should return 0", input_setup(n, argv, NULL) == 0);
    *(inotify_runnning()) = 0;
    return 0;
}
static char* test_handle_event()
{
    struct inotify_event e;
    int fd = inotify_init();
    memset(&e, 0, sizeof(struct inotify_event));;
    /* test empty event */ handle_event(&e, fd, NULL, NULL, NULL);
    e.len = 1;
    /* test with no event type */ handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_CREATE;
    handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_CREATE | IN_ISDIR;
    handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_MODIFY;
    handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_MODIFY | IN_ISDIR;
    handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_DELETE;
    handle_event(&e, fd, NULL, NULL, NULL);
    e.mask = IN_DELETE | IN_ISDIR;
    handle_event(&e, fd, NULL, NULL, NULL);
    return 0;
}
static char* test_get_event_path()
{
    struct inotify_event e;
    memset(&e, 0, sizeof(struct inotify_event));;
    mu_assert("event path should be NULL", 
            get_event_path(&e, NULL) == NULL);
    return 0;
}
static char* test_teardown()
{
    struct inotify_event e;
    memset(&e, 0, sizeof(struct inotify_event));;
    teardown_watches(NULL, 0, NULL);
    handle_file_deleted(&e, NULL, NULL, NULL);
    return 0;
}
void touch(char* path, char* str)
{
    FILE* f = fopen(path, "w");
    fwrite(str, strlen(str), 1, f);
    fclose(f);
}
static char* test_handle_file_modified()
{
    struct inotify_event* e = malloc(sizeof(struct inotify_event)
            + 42 /* for name flexattr */);
    memset(e, 0, sizeof(struct inotify_event));;
    handle_file_modified(e, NULL, NULL, NULL);
    fk_hash* watches = fk_hash_new();
    char* tmp = "/tmp";
    int fd = opendir(tmp);
    e->wd = watch_directory(tmp, fd, watches);
    char* path = "inotify_test_tmp";
    char* full_path = concat(tmp, path);
    touch(full_path, "test");
    strcpy(e->name, path);
    e->len = strlen(path);
    handle_file_modified(e, NULL, watches, tmp);
    closedir(fd);
    teardown_watches(NULL, fd, watches);
    free(e);
    unlink(full_path);
    free(full_path);
    return 0;

}
static char* test_on_event()
{
    struct inotify_event e;
    memset(&e, 0, sizeof(struct inotify_event));;
    on_event(NULL, 0, NULL, 0, NULL, NULL);
    e.len = 1;
    on_event(&e, 1, NULL, 0, NULL, NULL);
    return 0;
}
static char* all_tests()
{
    mu_run_test(test_input_setup);
    mu_run_test(test_handle_event);
    mu_run_test(test_teardown);
    mu_run_test(test_get_event_path);
    mu_run_test(test_handle_file_modified);
    mu_run_test(test_on_event);
    unlink(INOTIFY_INIT_PATH);
    return 0;
}
#include "minunit.c"
// LCOV_EXCL_STOP


