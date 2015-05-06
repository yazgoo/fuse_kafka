#ifndef STRING_LIST
#define STRING_LIST
typedef struct {
    char** array;
    int size, max_size;
} string_list;
int string_list_add_once(string_list** servers, char* string);
void string_list_free(string_list** servers);
#endif
