#include <input_plugin.h>
requires(glib-2.0)
#include <sys/inotify.h>
#include <glib.h>
#include <stdio.h>
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
char* get_event_path(struct inotify_event* event, GHashTable* watches)
{
    printf("new event on %d\n", event->wd);
    return concat((char*) g_hash_table_lookup(watches, (void*) event->wd), event->name);
}
handle_file_modified(struct inotify_event* event, GHashTable* offsets, GHashTable* watches, char* root)
{
    char* path = get_event_path(event, watches);
    int offset = (int) g_hash_table_lookup(offsets, (void*) path);
    printf("File %s modified.\n", path);
    char* line = 0;
    size_t length;
    FILE* f = fopen(path, "r");
    fseek(f, offset, SEEK_SET);
    ssize_t size;
    while((size = getline(&line, &length, f)) > 0)
        actual_kafka_write(path + strlen(root) - 1, line, size + 1, 0);
    g_hash_table_insert(offsets, path, (void*) ftell(f));
    fclose(f);
    free(path);
    free(line);
}
handle_file_deleted(struct inotify_event* event, GHashTable* offsets, GHashTable* watches, char* root)
{
    char* path = get_event_path(event, watches);
    g_hash_table_remove(offsets, path);
    free(path);
}
void watch_directory(char* directory, int fd, GHashTable* watches)
{
    int wd = inotify_add_watch(fd, directory, IN_CREATE | IN_MODIFY);
    printf("watching directory %s (%d)\n", directory, wd);
    g_hash_table_insert(watches, (void*) wd, directory);
}
void setup_watches(char* directory, int fd, GHashTable* watches)
{
    watch_directory(directory, fd, watches);
    DIR* dir = opendir(directory);
    struct dirent* file;
    printf("reading %s\n", directory);
    while(file = readdir(dir))
    {
        printf("%s %d\n", file->d_name, file->d_type);
        if(file->d_type == DT_DIR && strcmp(".", file->d_name) && strcmp("..", file->d_name))
        {
            char* path = concat(directory, file->d_name);
            setup_watches(path, fd, watches);
        }
    }
    closedir(dir);
}
void teardown_watches(char* directory, int fd, GHashTable* watches)
{
    /* TODO write */
    free(directory);
}
handle_event(struct inotify_event* event, int fd, GHashTable* offsets, GHashTable* watches, char* root)
{
    if ( event->len ) {
        if ( event->mask & IN_CREATE ) {
            if ( event->mask & IN_ISDIR ) {
                printf( "New directory %s created.\n", event->name );
                char* path = get_event_path(event, watches);
                setup_watches(path, fd, watches);
            }
            else {
                printf( "New file %s created.\n", event->name );
            }
        }
        if ( event->mask & IN_DELETE ) {
            if ( event->mask & IN_ISDIR ) {
                printf( "New directory %s deleted.\n", event->name );
                char* path = get_event_path(event, watches);
                teardown_watches(path, fd, watches);
            }
            else {
                printf( "New file %s deleted.\n", event->name );
                handle_file_deleted(event, offsets, watches, root);
            }
        }
        if ( event->mask & IN_MODIFY ) {
            if ( event->mask & IN_ISDIR ) {
                printf( "Directory %s modified.\n", event->name );
            }
            else {
                handle_file_modified(event, offsets, watches, root);
            }
        }
    }
}
int input_setup(int argc, char** argv, void* conf)
{
    fuse_get_context()->private_data = conf;
    fuse_get_context()->private_data = output_init((config*) conf);
    GHashTable* offsets = g_hash_table_new(NULL, NULL);
    GHashTable* watches = g_hash_table_new(NULL, NULL);
    char* directory = argv[1];
    int fd = inotify_init();
    setup_watches(directory, fd, watches);
    struct inotify_event event;
    char buffer[EVENT_BUF_LEN];
    int length; 
    while(length = read(fd, buffer, EVENT_BUF_LEN))
    {
        int i = 0;
        while ( i < length ) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            handle_event(event, fd, offsets, watches, directory);
            i += EVENT_SIZE + event->len;
        }
    }
    g_hash_table_foreach(watches, free, NULL);
    g_hash_table_destroy(offsets);
    g_hash_table_destroy(watches);
    return 0;
}
