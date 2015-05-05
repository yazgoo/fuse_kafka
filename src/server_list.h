#ifndef SERVER_LIST_H
#define SERVER_LIST_H
typedef struct {
    char** array;
    int size, max_size;
} server_list;
int server_list_contains(server_list** servers, char* string);
int server_list_add(server_list** servers, char* string);
#endif
