#include <input_plugin.h>
#include <windows.h>
#include <hash.c>
#include <handle_file_modified.c>
target(.*mingw.*)

void watch_directory(char* dir, config* conf)
{
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
        printf("received %d bytes\n", lpBytesReturned);
        if(lpBytesReturned == 0) continue;
        if(!(info[0].Action & FILE_ACTION_MODIFIED)) continue;
        char* path = (char*) malloc((info[0].FileNameLength + 1) * sizeof(char));
        if(path != NULL)
        {
            //wctombs(path, info[0].FileName, info[0].FileNameLength);
            snprintf(path, info[0].FileNameLength, "%S", info[0].FileName);
            path[info[0].FileNameLength] = 0; 
            printf("%s\n", path);
            char* full_path = concat(dir, path);
            free(path);
            if(full_path != NULL) handle_file_modified(full_path, offsets, "");
        }
    }
    fk_hash_delete(offsets, 1, 0);
}
int input_setup(int argc, char** argv, void* cfg)
{
    config* conf = (config*) cfg;
    for(conf->directory_n = 0; conf->directory_n < conf->directories_n;
            conf->directory_n++)
    {
        argv[1] = conf->directories[conf->directory_n];
        // TODO fork, otherwise, blocking API
        input_is_watching_directory(argv[1]);
        watch_directory(argv[1], conf);
    }
    return 0;
}
