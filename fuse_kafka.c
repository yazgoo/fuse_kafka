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
static int kafka_getattr(const char *path, struct stat *stbuf)
{
    int res;
    res = lstat(path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_fgetattr(const char *path, struct stat *stbuf,
            struct fuse_file_info *fi)
{
    int res;

    (void) path;

    res = fstat(fi->fh, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_access(const char *path, int mask)
{
    int res;

    res = access(path, mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_readlink(const char *path, char *buf, size_t size)
{
    int res;

    res = readlink(path, buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}

static int kafka_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp = opendir(path);
    if (dp == NULL)
        return -errno;

    fi->fh = (unsigned long) dp;
    return 0;
}

static inline DIR *get_dirp(struct fuse_file_info *fi)
{
    return (DIR *) (uintptr_t) fi->fh;
}
static int kafka_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi)
{
    DIR *dp = get_dirp(fi);
    struct dirent *de;

    (void) path;
    seekdir(dp, offset);
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, telldir(dp)))
            break;
    }

    return 0;
}

static int kafka_releasedir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp = get_dirp(fi);
    (void) path;
    closedir(dp);
    return 0;
}

static int kafka_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;

    if (S_ISFIFO(mode))
        res = mkfifo(path, mode);
    else
        res = mknod(path, mode, rdev);
    if (res == -1)
    {
        printf("%s ", path); perror("mknod ");
        return -errno;
    }

    return 0;
}

static int kafka_mkdir(const char *path, mode_t mode)
{
    int res;

    res = mkdir(path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_unlink(const char *path)
{
    int res;

    res = unlink(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_rmdir(const char *path)
{
    int res;

    res = rmdir(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_symlink(const char *from, const char *to)
{
    int res;

    res = symlink(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_rename(const char *from, const char *to)
{
    int res;

    res = rename(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_link(const char *from, const char *to)
{
    int res;

    res = link(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_chmod(const char *path, mode_t mode)
{
    int res;

    res = chmod(path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    res = lchown(path, uid, gid);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_truncate(const char *path, off_t size)
{
    int res;

    res = truncate(path, size);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_ftruncate(const char *path, off_t size,
             struct fuse_file_info *fi)
{
    int res;

    (void) path;

    res = ftruncate(fi->fh, size);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_utimens(const char *path, const struct timespec ts[2])
{
    int res;
    struct timeval tv[2];

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    res = utimes(path, tv);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int fd;

    fd = open(path, fi->flags, mode);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int kafka_open(const char *path, struct fuse_file_info *fi)
{
    int fd;

    fd = open(path, fi->flags);
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int kafka_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    int res;

    (void) path;
    res = pread(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;

    return res;
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

static int kafka_statfs(const char *path, struct statvfs *stbuf)
{
    int res;

    res = statvfs(path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_flush(const char *path, struct fuse_file_info *fi)
{
    int res;

    (void) path;
    /* This is called from every close on an open file, so call the
       close on the underlying filesystem.  But since flush may be
       called multiple times for an open file, this must not really
       close the file.  This is important if used on a network
       filesystem like NFS which flush the data/metadata on close() */
    res = close(dup(fi->fh));
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;
    close(fi->fh);

    return 0;
}

static int kafka_fsync(const char *path, int isdatasync,
             struct fuse_file_info *fi)
{
    int res;
    (void) path;

#ifndef HAVE_FDATASYNC
    (void) isdatasync;
#else
    if (isdatasync)
        res = fdatasync(fi->fh);
    else
#endif
        res = fsync(fi->fh);
    if (res == -1)
        return -errno;

    return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int kafka_setxattr(const char *path, const char *name, const char *value,
            size_t size, int flags)
{
    int res = lsetxattr(path, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int kafka_getxattr(const char *path, const char *name, char *value,
            size_t size)
{
    int res = lgetxattr(path, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int kafka_listxattr(const char *path, char *list, size_t size)
{
    int res = llistxattr(path, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int kafka_removexattr(const char *path, const char *name)
{
    int res = lremovexattr(path, name);
    if (res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static int kafka_lock(const char *path, struct fuse_file_info *fi, int cmd,
            struct flock *lock)
{
    /*(void) path;
    FILE* f = fopen("/tmp/log", "w");
    fprintf(f, "%d\n", cmd);
    fclose(f);
    return ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
               sizeof(fi->lock_owner));*/
    return 0;
}
/*
static int kafka_flock(const char *path, struct fuse_file_info *fi, int op)
{
    int res;
    (void) path;
    res = flock(fi->fh, op);
    if (res == -1) return -errno;
    return 0;
}
*/


static struct fuse_operations kafka_oper = {
    .init       = kafka_init,
    .getattr    = kafka_getattr,
    .fgetattr   = kafka_fgetattr,
    .access     = kafka_access,
    .readlink   = kafka_readlink,
    .opendir    = kafka_opendir,
    .readdir    = kafka_readdir,
    .releasedir = kafka_releasedir,
    .mknod      = kafka_mknod,
    .mkdir      = kafka_mkdir,
    .symlink    = kafka_symlink,
    .unlink     = kafka_unlink,
    .rmdir      = kafka_rmdir,
    .rename     = kafka_rename,
    .link       = kafka_link,
    .chmod      = kafka_chmod,
    .chown      = kafka_chown,
    .truncate   = kafka_truncate,
    .ftruncate  = kafka_ftruncate,
    .utimens    = kafka_utimens,
    .create     = kafka_create,
    .open       = kafka_open,
    .read       = kafka_read,
    .write      = kafka_write,
    .statfs     = kafka_statfs,
    .flush      = kafka_flush,
    .release    = kafka_release,
    .fsync      = kafka_fsync,
#ifdef HAVE_SETXATTR
    .setxattr   = kafka_setxattr,
    .getxattr   = kafka_getxattr,
    .listxattr  = kafka_listxattr,
    .removexattr    = kafka_removexattr,
#endif
    .lock       = kafka_lock,
    /*.flock      = kafka_flock,*/
};
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
