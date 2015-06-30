#define FUSE_USE_VERSION 26
#include <fuse.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <input_plugin.h>
target(.*linux.*)
requires(fuse)
/**
 * @brief write the data to kafka and to the overlaid fs if it should
 * be done
 * @param path file path to save to kafka
 * @param buf write buffer
 * @param size size of the buffer to write
 * @param fi file information @see fuse
 * @return @see pwrite
 */
static int kafka_write(const char *path, const char *buf,
        size_t size, off_t offset, struct fuse_file_info *fi)
{
    int res;
    output_write(*(input_get_last_watching_directory()),
            path, buf, size, offset);
    DO_AS_CALLER(
            res = pwrite(fi->fh, buf, size, offset);
    )
    if (res == -1)
        res = -errno;

    return res;
}
static int kafka_getattr(const char *path, struct stat *stbuf)
{
    int res;
    DO_AS_CALLER(
            res = lstat(path, stbuf);
    )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_fgetattr(const char *path, struct stat *stbuf,
            struct fuse_file_info *fi)
{
    int res;

    (void) path;

    DO_AS_CALLER(
            res = fstat(fi->fh, stbuf);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_access(const char *path, int mask)
{
    int res;

    DO_AS_CALLER(
            res = access(path, mask);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_readlink(const char *path, char *buf, size_t size)
{
    int res;

    DO_AS_CALLER(
            res = readlink(path, buf, size - 1);
            )
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}

static int kafka_opendir(const char *path, struct fuse_file_info *fi)
{
    DO_AS_CALLER(
            DIR *dp = opendir(path);
            )
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
    DO_AS_CALLER(
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
        )

    return 0;
}

static int kafka_releasedir(const char *path, struct fuse_file_info *fi)
{
    DO_AS_CALLER(
            DIR *dp = get_dirp(fi);
            (void) path;
            closedir(dp);
            )
    return 0;
}

static int kafka_mknod(const char *path, mode_t mode, dev_t rdev)
{
    DO_AS_CALLER(
            int res;

            if (S_ISFIFO(mode))
            res = mkfifo(path, mode);
            else
            res = mknod(path, mode, rdev);
            )
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

    DO_AS_CALLER(
            res = mkdir(path, mode);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_unlink(const char *path)
{
    int res;

    DO_AS_CALLER(
            res = unlink(path);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_rmdir(const char *path)
{
    int res;

    DO_AS_CALLER(
            res = rmdir(path);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_symlink(const char *from, const char *to)
{
    int res;

    DO_AS_CALLER(
            res = symlink(from, to);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_rename(const char *from, const char *to)
{
    int res;

    DO_AS_CALLER(
            res = rename(from, to);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_link(const char *from, const char *to)
{
    int res;

    DO_AS_CALLER(
            res = link(from, to);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_chmod(const char *path, mode_t mode)
{
    int res;

    DO_AS_CALLER(
            res = chmod(path, mode);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    DO_AS_CALLER(
            res = lchown(path, uid, gid);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_truncate(const char *path, off_t size)
{
    int res;

    DO_AS_CALLER(
            res = truncate(path, size);
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_ftruncate(const char *path, off_t size,
             struct fuse_file_info *fi)
{
    int res;

    (void) path;

    DO_AS_CALLER(
            res = ftruncate(fi->fh, size);
            )
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

    DO_AS_CALLER(
            fd = open(path, fi->flags, mode);
            )
    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

static int kafka_open(const char *path, struct fuse_file_info *fi)
{
    int fd;

    DO_AS_CALLER(
            fd = open(path, fi->flags);
            )
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
    DO_AS_CALLER(
            res = pread(fi->fh, buf, size, offset);
            )
    if (res == -1)
        res = -errno;

    return res;
}
static int kafka_statfs(const char *path, struct statvfs *stbuf)
{
    int res;

    DO_AS_CALLER(
            res = statvfs(path, stbuf);
            )
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
    DO_AS_CALLER(
            res = close(dup(fi->fh));
            )
    if (res == -1)
        return -errno;

    return 0;
}

static int kafka_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;
    DO_AS_CALLER(
            close(fi->fh);
            )

    return 0;
}

static int kafka_fsync(const char *path, int isdatasync,
             struct fuse_file_info *fi)
{
    int res;
    (void) path;
DO_AS_CALLER(
#ifndef HAVE_FDATASYNC
    (void) isdatasync;
#else
    if (isdatasync)
        res = fdatasync(fi->fh);
    else
#endif
        res = fsync(fi->fh);
        )
    if (res == -1)
        return -errno;

    return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int kafka_setxattr(const char *path, const char *name, const char *value,
            size_t size, int flags)
{
    DO_AS_CALLER(
            int res = lsetxattr(path, name, value, size, flags);
            )
    if (res == -1)
        return -errno;
    return 0;
}

static int kafka_getxattr(const char *path, const char *name, char *value,
            size_t size)
{
    DO_AS_CALLER(
            int res = lgetxattr(path, name, value, size);
            )
    if (res == -1)
        return -errno;
    return res;
}

static int kafka_listxattr(const char *path, char *list, size_t size)
{
    DO_AS_CALLER(
            int res = llistxattr(path, list, size);
            )
    if (res == -1)
        return -errno;
    return res;
}

static int kafka_removexattr(const char *path, const char *name)
{
    DO_AS_CALLER(
            int res = lremovexattr(path, name);
            )
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
void* kafka_init(struct fuse_conn_info *conn)
{
    /* TODO remove output_init */
    return output_init((config*) fuse_get_context()->private_data);
    //return fuse_get_context()->private_data;
}


static struct fuse_operations kafka_oper = {
    .init       = kafka_init,
    .destroy    = output_destroy,
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

int input_setup(int argc, char** argv, void* cfg)
{
    config* conf = (config*) cfg;
    for(conf->directory_n = 0; conf->directory_n < conf->directories_n;
            conf->directory_n++)
    {
        argv[1] = conf->directories[conf->directory_n];
        if(!fork())
        {
#ifdef TEST
            break;
#endif
            conf->directory_fd = open(conf->directories[conf->directory_n],
                    O_RDONLY);
            input_is_watching_directory(argv[1]);
            return argv == NULL? -1 : fuse_main(argc, argv, &kafka_oper, conf);
        }
    }
    return 0;
}
