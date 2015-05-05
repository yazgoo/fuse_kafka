#include "server_list.h"
#define SERVER_LIST_DEFAULT_MAX_SIZE 10
int server_list_contains(server_list** servers, char* string)
{
    int i;
    if((*servers) == NULL) return 0;
    for(i = 0; i < (*servers)->size; i++)
        if(strcmp((*servers)->array[i], string) == 0) return 1;
    return 0;
}
int server_list_new(server_list** servers)
{
    if(((*servers) = malloc(sizeof(server_list))) == NULL) return 1;
    (*servers)->max_size = SERVER_LIST_DEFAULT_MAX_SIZE;
    (*servers)->size = 0;
    if(((*servers)->array = calloc((*servers)->max_size, sizeof(char*)))
            == NULL) return 2;
    return 0;
}
void server_list_free(server_list** servers)
{
    if((*servers) == NULL) return;
    free((*servers)->array);
    (*servers)->array = NULL;
    free((*servers));
}
int server_list_resize(server_list** servers)
{
    int new_size = (*servers)->max_size + SERVER_LIST_DEFAULT_MAX_SIZE;
    char** new_array;
    if((new_array = (char**) realloc((*servers)->array,
                    new_size * sizeof(char*))) == NULL) return 1;
    (*servers)->max_size = new_size;
    return 0;
}
int server_list_add(server_list** servers, char* string)
{
    char* str;
    if((*servers) == NULL && server_list_new(servers)) return 1;
    if((*servers)->size >= (*servers)->max_size && server_list_resize(servers))
        return 2;
    if((str = (char*) calloc(strlen(string) + 1, sizeof(char))) == NULL)
        return 3;
    (*servers)->array[(*servers)->size] = str;
    strcpy((*servers)->array[(*servers)->size++], string);
    return 0;
}
