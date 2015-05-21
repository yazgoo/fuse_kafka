#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define FK_HASH_SIZE 100
typedef struct _fk_hash_list {
    void* key;
    void* value;
    struct _fk_hash_list* next;
} fk_hash_list;
typedef fk_hash_list** fk_hash;
fk_hash fk_hash_new()
{
    fk_hash hash = (fk_hash) malloc(sizeof(fk_hash_list*) * FK_HASH_SIZE);
    memset(hash, 0, sizeof(fk_hash_list*) * FK_HASH_SIZE);
}
/**
 * @returns a hash value corresponding to a key
 * @param key_is_string 1 if value to hash is a string, 0 if it is an integer
 */
int fk_hash_hash(void* key, int key_is_string)
{
    char* k = (char*) key;
    long int i = (long int) key;
    if(key_is_string && key == NULL) return 0;
    return ((key_is_string? (k[0] + k[strlen(k) - 1]): (i < 0? -i : i)) % FK_HASH_SIZE);
}
/**
 * @brief allocates and initializes a new hash item
 */
fk_hash_list* fk_hash_list_new(void* key, void* value)
{
    char* k = (char*) key;
    fk_hash_list* list = malloc(sizeof(fk_hash_list));
    list->key = key; 
    list->value = value;
    list->next = 0;
    return list;
}
/**
 * @brief put an item in the hash
 * @param key_is_string 1 if key is string, 0 if it is an integer (used for hashing)
 */
void fk_hash_put(fk_hash hash, void* key, void* value, int key_is_string)
{
    if(hash == NULL) return;
    int h = fk_hash_hash(key, key_is_string);
    if(hash[h] == NULL) hash[h] = fk_hash_list_new(key, value);
    else
    {
        fk_hash_list* current;
        for(current = hash[h]; current->next != NULL; current = current->next)
            if((key_is_string && strcmp(current->key, key) == 0) || (!key_is_string && current->key == key))
            {
                current->value = value;
                return;
            }
        current->next = fk_hash_list_new(key, value);
    }
}
/**
 * @brief put an item in the hash
 * @param key_is_string 1 if key is string, 0 if it is an integer (used for hashing)
 */
void* fk_hash_get(fk_hash hash, void* key, int key_is_string)
{
    if (hash == NULL) return (void*) -1;
    int h = fk_hash_hash(key, key_is_string);
    if(hash[h] == NULL) return (void*) -1;
    fk_hash_list* current;
    for(current = hash[h]; current != NULL; current = current->next)
    {
        if((key_is_string && strcmp(current->key, key) == 0) || (!key_is_string && current->key == key))
        {
            return current->value;
        }
    }
    return (void*) -1;
}
void fk_hash_remove(fk_hash hash, void* key, int key_is_string)
{
    if(hash == NULL) return;
    int h = fk_hash_hash(key, key_is_string);
    if(hash[h] == NULL) return;
    fk_hash_list* current, *previous;
    previous = NULL;
    for(current = hash[h]; current != NULL; current = current->next)
    {
        if((key_is_string && strcmp(current->key, key) == 0) || (!key_is_string && current->key == key))
        {
            if(previous == NULL) hash[h] = NULL;
            else previous->next = current->next;
            free(current);
            return;
        }
        previous = current;
    }
}
void fk_hash_list_delete(fk_hash_list* list, int delete_keys, int delete_value)
{
    if(list == NULL) return;
    fk_hash_list_delete(list->next, delete_keys, delete_value);
    if(delete_keys) free(list->key);
    if(delete_value) free(list->value);
    free(list);
}
void fk_hash_delete(fk_hash hash, int delete_keys, int delete_value)
{
    int i;
    if(hash == NULL) return;
    for(i = 0; i < FK_HASH_SIZE; i++)
    {
        fk_hash_list_delete(hash[i], delete_keys, delete_value);
        hash[i] = NULL;
    }
    free(hash);
}
