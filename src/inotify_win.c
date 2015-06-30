struct inotify_event
{
    int wd;
    int mask;
    int len;
    char* name;
}
#define IN_MODIFY FILE_NOTIFY_CHANGE_SIZE
#define IN_CREATE FILE_NOTIFY_CHANGE_FILE_NAME
#define IN_DELETE FILE_NOTIFY_CHANGE_FILE_NAME
#define IN_ISDIR FILE_NOTIFY_CHANGE_FILE_NAME
;
int inotify_init(void)
{
    return 0;
}
int inotify_add_watch (int __fd, const char *__name, uint32_t __mask)
{
    return 0;
}

