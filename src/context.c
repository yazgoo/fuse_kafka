#ifndef _FUSE_H_
struct fuse_context {
	int uid;
	int gid;
	int pid;
	void *private_data;
};
struct fuse_context context;
struct fuse_context* fuse_get_context()
{
    return &context;
}
#endif
