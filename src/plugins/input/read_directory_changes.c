#include <input_plugin.h>
#include <pthread.h>
#include <windows.h>
#include <hash.c>
#include <handle_file_modified.c>
target(".*mingw.*")

void watch_directory(char* dir, config* conf)
{
    trace_debug("rdc watch_directory: CreateFile dir %s", dir);
    char prefix[] = {dir[0], 0};
    fk_hash offsets = fk_hash_new();
    HANDLE hDir = CreateFile(
            dir,
            FILE_LIST_DIRECTORY,
            FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
            NULL, 
            OPEN_EXISTING, 
            FILE_FLAG_BACKUP_SEMANTICS, 
            NULL
            );
    FILE_NOTIFY_INFORMATION info[1024];
    DWORD lpBytesReturned;
    trace_debug("rdc watch_directory: starting ReadDirectoryChangesW loop");
    while(1)
    {
        if(!ReadDirectoryChangesW(
                hDir,
                &info,
                sizeof(info),
                1,
                FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME,
                &lpBytesReturned,
                NULL,
                NULL
                )) continue;
        if(lpBytesReturned == 0) continue;
        if(!(info[0].Action & FILE_ACTION_MODIFIED)) continue;
        char* path = (char*) malloc((info[0].FileNameLength + 1) * sizeof(char));
        trace_debug("rdc watch_directory: file modified", path);
        if(path != NULL)
        {
            snprintf(path, info[0].FileNameLength, "%S", info[0].FileName);
            path[info[0].FileNameLength] = 0; 
            trace_debug("rdc watch_directory: file modified is %s", path);
            char* full_path = concat(dir, path);
            trace_debug("rdc watch_directory: file modified is %s", full_path);
            free(path);
            if(full_path != NULL)
            {
                trace_debug("rdc watch_directory: calling handle_file_modified(%s, %d, %s)", full_path, offsets, prefix);
                handle_file_modified(full_path, offsets, prefix);
            }
        }
    }
    fk_hash_delete(offsets, 1, 0);
}
typedef struct _name_and_conf
{
	char* name;
	config* conf;
}name_and_conf;
void start_watching_directory(name_and_conf* tuple)
{
	trace_debug("rdc input setup: watching dir %s", tuple->name );
	input_is_watching_directory(tuple->name);
	trace_debug("rdc input setup: launching watch_dir %s", tuple->name );
	watch_directory(tuple->name, tuple->conf);
}
int input_setup(int argc, char** argv, void* cfg)
{
    trace_debug("rdc input setup: looping through configs");
    config* conf = (config*) cfg;
    pthread_t* threads = calloc(conf->directories_n, sizeof(pthread_t));
    pthread_t* tuples = calloc(conf->directories_n, sizeof(name_and_conf));
    for(conf->directory_n = 0; conf->directory_n < conf->directories_n;
            conf->directory_n++)
    {
	name_and_conf* tuple = tuples + conf->directory_n;
	tuple->name = conf->directories[conf->directory_n];
	tuple->conf = conf;
	pthread_create(threads + conf->directory_n,
			NULL, start_watching_directory, (void*) tuple);
    }
    for(conf->directory_n = 0; conf->directory_n < conf->directories_n;
            conf->directory_n++)
    {
	    pthread_join(threads + conf->directories_n, NULL);
    }
    free(threads);
    free(tuples);
    return 0;
}
