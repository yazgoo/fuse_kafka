#define fuse_get_context(a) test_fuse_get_context(a)
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

