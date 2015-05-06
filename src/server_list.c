/** @file
 * Allows to maintain a server list to check
 * that these servers are not added;
 * Currently, there is no parsing of the string argument given.
 * In the future there might be (to handle server lists) ala zookeeper
 **/
#include "server_list.h"
#define SERVER_LIST_DEFAULT_MAX_SIZE 10
/** @brief Public: checks if the list contains a string
 *
 * @param servers a pointer to the server_list
 * 
 * @return 0 if *servers is NULL or the list does 
 * not contain the string, 1 otherwise */
int server_list_contains(server_list** servers, char* string)
{
    int i;
    if((*servers) == NULL)
        return 0;
    for(i = 0; i < (*servers)->size; i++)
        if(strcmp((*servers)->array[i], string) == 0)
            return 1;
    return 0;
}
/** @brief creates a new server_list
 * @return 0 if the creation was a success
 **/
int server_list_new(server_list** servers)
{
    if(((*servers) = fmalloc(sizeof(server_list))) == NULL)
        return 1;
    (*servers)->max_size = SERVER_LIST_DEFAULT_MAX_SIZE;
    (*servers)->size = 0;
    if(((*servers)->array = fcalloc((*servers)->max_size, sizeof(char*)))
            == NULL)
        return 2;
    return 0;
}
/** @brief deletes the server list **/
void server_list_free(server_list** servers)
{
    int i;
    if((*servers) == NULL)
        return;
    if((*servers)->array != NULL)
        for(i = 0; i < (*servers)->size; i++)
            free((*servers)->array[i]);
    free((*servers)->array);
    (*servers)->array = NULL;
    free((*servers));
}
/**
 * @brief resizes the server list
 * @return 1 if resize failed, 0 otherwise
 **/
int server_list_resize(server_list** servers)
{
    int new_size = (*servers)->max_size + SERVER_LIST_DEFAULT_MAX_SIZE;
    char** new_array;
    if((new_array = (char**) frealloc((*servers)->array,
                    new_size * sizeof(char*))) == NULL)
        return 1;
    (*servers)->max_size = new_size;
    (*servers)->array = new_array;
    return 0;
}
/**
 * @brief adds a string to the servers list
 * @return 0 if adding was successfull
 **/
int server_list_add(server_list** servers, char* string)
{
    char* str;
    if((*servers) == NULL && server_list_new(servers))
        return 1;
    if((*servers)->size >= (*servers)->max_size &&
            server_list_resize(servers))
        return 2;
    if((str = (char*) fcalloc(strlen(string) + 1, sizeof(char))) == NULL)
        return 3;
    (*servers)->array[(*servers)->size] = str;
    strcpy((*servers)->array[(*servers)->size++], string);
    return 0;
}
