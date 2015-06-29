#include <input_plugin.h>
#include <hash.c>
#include <dirent.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <pthread.h>
requires(pthread)
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )
char* get_event_path(struct inotify_event* event, fk_hash watches)
{
    // printf("new event on %d\n", event->wd);
    char* a = (char*) fk_hash_get(watches, (void*) (long int) event->wd, 0);
    if(a == (char*) -1) a = NULL;
    return concat(a, event->name);
}
void handle_file_modified(struct inotify_event* event, fk_hash offsets, fk_hash watches, char* root)
{
    char* path = get_event_path(event, watches);
    char* old_path = path;
    if(path == NULL) return;
    long int offset = (long int) fk_hash_get(offsets, path, 1);
    if(offset == -1)
    {
        path = strdup(path);
        offset = 0;
    }
    // printf("File %s modified, offset being %ld.\n", path, offset);
    char* line = 0;
    size_t length;
    FILE* f = fopen(path, "r");
    if(f != NULL)
    {
        fseek(f, 0, SEEK_END); if(ftell(f) < offset) offset = 0;
        fseek(f, offset, SEEK_SET);
        ssize_t size;
        while((size = getline(&line, &length, f)) > 0)
        {
            // printf("File %s, writing %s\n", path, line);
            output_write("/", path, line, size, 0);
        }
        if(ftell(f) > offset)
        {
            // printf("File %s started reading @%ld, ended @%ld.\n", path, offset, ftell(f));
        }
        fk_hash_put(offsets, old_path, (void*) ftell(f), 1);
        fclose(f);
    }
    else
        if(old_path != path) free(old_path);
    free(path);
    free(line);
}
handle_file_deleted(struct inotify_event* event, fk_hash offsets, fk_hash watches, char* root)
{
    char* path = get_event_path(event, watches);
    fk_hash_remove(offsets, path, 1, 0, 1);
    free(path);
}
handle_file_created(struct inotify_event* event, fk_hash offsets, fk_hash watches, char* root)
{
    // printf( "New file %s created.\n", event->name );
    /*
    char* path = get_event_path(event, watches);
    fk_hash_put(offsets, path, 0, 1);
    printf( "New file %s created.\n", path );
    free(path);
    */
}
int watch_directory(char* directory, int fd, fk_hash watches)
{
    int wd = inotify_add_watch(fd, directory, IN_CREATE | IN_MODIFY);
    // printf("watching directory %s (%d)\n", directory, wd);
    fk_hash_put(watches, (void*) (long int) wd, strdup(directory), 0);
    return wd;
}
void setup_offset(char* path, fk_hash offsets)
{
    FILE* f = fopen(path, "r");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        fk_hash_put(offsets, path, (void*) ftell(f), 1);
        fclose(f);
    }
    else
    {
        free(path);
    }
}
void setup_watches(char* directory, int fd, fk_hash watches, fk_hash offsets)
{
    if(directory == NULL) return;
    watch_directory(directory, fd, watches);
    DIR* dir = opendir(directory);
    if(dir == NULL) return;
    struct dirent* file;
    // printf("reading %s\n", directory);
    while(file = readdir(dir))
    {
        // printf("%s %d\n", file->d_name, file->d_type);
        char* path = concat(directory, file->d_name);
        if(file->d_type == DT_DIR && strcmp(".", file->d_name) && strcmp("..", file->d_name))
        {
#ifndef INOTIFY_NONRECURSIVE
            setup_watches(path, fd, watches, offsets);
#endif
        }
        else
        {
            setup_offset(path, offsets);
        }
    }
    closedir(dir);
}
void teardown_watches(char* directory, int fd, fk_hash watches)
{
    /* TODO write */
    free(directory);
}
handle_event(struct inotify_event* event, int fd, fk_hash offsets, fk_hash watches, char* root)
{
    if ( event->len ) {
        if ( event->mask & IN_CREATE ) {
            if ( event->mask & IN_ISDIR ) {
                // printf( "New directory %s created.\n", event->name );
#ifndef INOTIFY_NONRECURSIVE
                char* path = get_event_path(event, watches);
                setup_watches(path, fd, watches, offsets);
#endif
            }
            else {
                handle_file_created(event, offsets, watches, root);
            }
        }
        if ( event->mask & IN_DELETE ) {
            if ( event->mask & IN_ISDIR ) {
                // printf( "New directory %s deleted.\n", event->name );
                char* path = get_event_path(event, watches);
                teardown_watches(path, fd, watches);
            }
            else {
                // printf( "New file %s deleted.\n", event->name );
                handle_file_deleted(event, offsets, watches, root);
            }
        }
        if ( event->mask & IN_MODIFY ) {
            if ( event->mask & IN_ISDIR ) {
                // printf( "Directory %s modified.\n", event->name );
            }
            else {
                handle_file_modified(event, offsets, watches, root);
            }
        }
    }
}
void on_event(char* buffer, int length, char* directory, int fd, fk_hash offsets, fk_hash watches)
{
    int i = 0;
    while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        handle_event(event, fd, offsets, watches, directory);
        i += EVENT_SIZE + event->len;
    }
}
int* inotify_runnning()
{
    static int value = 1;
    if(value < 0) value++;
    return &value;
}
int input_setup(int argc, char** argv, void* cfg)
{
    config* conf = (config*) cfg;
    fk_hash offsets = fk_hash_new();
    fk_hash watches = fk_hash_new();
    int fd = inotify_init();
    if(conf != NULL)
    {
        for(conf->directory_n = 0; conf->directory_n < conf->directories_n;
                conf->directory_n++)
        {
            input_is_watching_directory(conf->directories[conf->directory_n]);
            setup_watches(conf->directories[conf->directory_n], fd, watches, offsets);
        }
        conf->directories[conf->directory_n] = "/"; /* TODO fix this bypass in output.c */
    }
    struct inotify_event event;
    char buffer[EVENT_BUF_LEN];
    int length; 
    while(*(inotify_runnning()) && (length = read(fd, buffer, EVENT_BUF_LEN)))
    {
        on_event(buffer, length, NULL, fd, offsets, watches);
    }
    fk_hash_delete(offsets, 1, 0);
    fk_hash_delete(watches, 0, 1);
    return 0;
}
