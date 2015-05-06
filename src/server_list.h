#ifndef SERVER_LIST_H
#define SERVER_LIST_H
#include "string_list.h"
typedef string_list server_list;
int server_list_add_once(server_list** servers, char* string);
#define server_list_free string_list_free
#endif
