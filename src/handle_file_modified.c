void handle_file_modified(char* path, fk_hash offsets, char* root)
{
    char* old_path = path;
    if(path == NULL) return;
    long int offset = (long int) fk_hash_get(offsets, path, 1);
    if(offset == -1)
    {
        path = strdup(path);
        offset = 0;
    }
    printf("File %s modified, offset being %ld.\n", path, offset);
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
            printf("File %s, writing %s %d\n", path, line, size);
            output_write("/", path, line, size, 0);
        }
        if(ftell(f) > offset)
        {
            printf("File %s started reading @%ld, ended @%ld.\n", path, offset, ftell(f));
        }
        fk_hash_put(offsets, old_path, (void*) ftell(f), 1);
        fclose(f);
    }
    else
        if(old_path != path) free(old_path);
    free(path);
    free(line);
}

