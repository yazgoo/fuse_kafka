/** @file
 * Allows to maintain a server list to check
 * that these servers are not added;
 * Currently, there is no parsing of the string argument given.
 * In the future there might be (to handle server lists) ala zookeeper
 **/
#include "string_list.h"
#define SERVER_LIST_DEFAULT_MAX_SIZE 10
/** @brief Public: checks if the list contains a string
 *
 * @param servers a pointer to the string_list
 * 
 * @return 0 if *servers is NULL or the list does 
 * not contain the string, 1 otherwise */
int string_list_contains(string_list** servers, char* string)
{
    int i;
    if((*servers) == NULL)
        return 0;
    for(i = 0; i < (*servers)->size; i++)
    {
        if(strcmp((*servers)->array[i], string) == 0)
            return 1;
    }
    return 0;
}
/** @brief creates a new string_list
 * @return 0 if the creation was a success
 **/
int string_list_new(string_list** servers)
{
    if(((*servers) = fmalloc(sizeof(string_list))) == NULL)
        return 1;
    (*servers)->max_size = SERVER_LIST_DEFAULT_MAX_SIZE;
    (*servers)->size = 0;
    if(((*servers)->array = fcalloc((*servers)->max_size, sizeof(char*)))
            == NULL)
        return 2;
    return 0;
}
/** @brief deletes the server list **/
void string_list_free(string_list** servers)
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
int string_list_resize(string_list** servers)
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
int string_list_add(string_list** servers, char* string)
{
    char* str;
    if((*servers) == NULL && string_list_new(servers))
        return 1;
    if((*servers)->size >= (*servers)->max_size &&
            string_list_resize(servers))
        return 2;
    if((str = (char*) fcalloc(strlen(string) + 1, sizeof(char))) == NULL)
        return 3;
    (*servers)->array[(*servers)->size] = str;
    strcpy((*servers)->array[(*servers)->size], string);
    (*servers)->size++;
    return 0;
}
/** @brief add item to list if it is not in it already
 * 
 * @return 0 if item already is in the list
 *         2 if item is not in the list but could
 *               not be added
 *         1 if item was not in the list and was successfully added
 */
int string_list_add_once(string_list** servers, char* string)
{
    if(string_list_contains(servers, string))
        return 0;
    if(string_list_add(servers, string))
        return 2;
    return 1;
}
