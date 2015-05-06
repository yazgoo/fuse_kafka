#include "server_list.h"
#include "string_list.c"
/** @brief add item to list if it is not in it already
 * 
 * @param a comma separated server list ala zookeeper:
 *        memory must be writable
 * 
 * @return a logical or of all the string_list_add_once calls 
 */
int server_list_add_once(server_list** list, char* servers)
{
    char* server = servers;
    int result = 0, zero = 0;
    int i;
    int length;
    if(servers == NULL) return 0;
    length = strlen(servers);
    for(i = 0; i <= length; i++)
        if((zero = !servers[i]) || servers[i] == ',')
        {
            if(!zero) servers[i] = 0;
            result |= string_list_add_once(list, server);
            if(!zero) servers[i] = ',';
            server = servers + i + 1;
        }
    return result;
}
