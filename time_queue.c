#include <stdlib.h>
#include <stdio.h>
typedef struct
{
    unsigned long* values;
    unsigned long* hashes;
    unsigned int size, i, quota;
}
time_queue;
/* Internal: generates a djb2 hash 
 *
 * str - the string to hash
 * 
 * Examples
 *
 *     time_queue_hash('hello world')
 *          => 13876786532495509697
 *
 * Return a hash for the string */
unsigned long time_queue_hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++) hash = ((hash << 5) + hash) + c;
    return hash;
}
/* Public: instantiates a new time queue, free it with
 * time_queue_delete
 *
 * size  - the maximum number of items saved in the queue
 * quota - the maximum size allowed per second per hash entry
 *
 * Examples
 *
 *      time_queue_new(10, 42)
 *          => time_queue*
 *
 * Return a newly allocated pointer to a time queue of the specified
 * size with the specified quota initialized to zero for values, and
 * hashes */
time_queue* time_queue_new(unsigned int size, unsigned int quota)
{
    int i;
    time_queue* queue = (time_queue*) malloc(sizeof(time_queue));
    queue->values = (unsigned long*) malloc(size * sizeof(long*));
    queue->hashes = (unsigned long*) malloc(size * sizeof(long*));
    queue->size = size;
    queue->quota = quota;
    queue->i = 0;
    for(i = 0; i < size; i++)
    {
        queue->values[i] = queue->hashes[i] = 0;
    }
    return queue;
}
/* Internal: get the time
 *
 * Return current time in microseconds */
unsigned long time_queue_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
/* Public: set a time queue key to current time
 *
 * queue    - the time queue to modify
 * key      - the key to set
 * 
 * Examples
 *
 *      time_queue_set(queue, "/var/log/blah.log")
 */
void time_queue_set(time_queue* queue, char* key)
{
    queue->i += 1;
    queue->i = queue->i % queue->size;
    queue->hashes[queue->i] = time_queue_hash(key);
    queue->values[queue->i] = time_queue_time();
}
/* Public: get last stored time pointer for given key
 *
 * queue    - the time queue to modify
 * key      - the key to get the value from
 *
 * Examples
 *
 *     time_queue_get(queue, "/unset/key") 
 *          => NULL
 *     time_queue_get(queue, "/existing/key")
 *          => pointer to: 1412074060579654
 *
 * Return a pointer to the time value registered, NULL if there is
 * none */
unsigned long* time_queue_get(time_queue* queue, char* key)
{
    int i;
    unsigned long hash = time_queue_hash(key);
    for(i = 0; i < queue->size; i++)
    {
        if(hash == queue->hashes[i])
            return queue->values + i;
    }
    return NULL;
}
/* Internal: checks if the given key overflows the quota
 *
 * queue    - the time queue to get data from
 * key      - the key to check
 * size     - the size to compare with the quota
 *
 * Examples
 * 
 *   time_queue* queue = time_queue_new(10, 42);
 *   time_queue_set(queue, "/var/log/lol");
 *   sleep(1);
 *   time_queue_overflows(queue, "/var/log/lol", 880);
 *      => 1
 *
 * Return 0 if the key does not overflow the quota or no value exists
 * for this key, 1 overwise */
int time_queue_overflows(time_queue* queue, char* key, unsigned int size)
{
    unsigned long* time = time_queue_get(queue, key);
    if(time == NULL) return 0;
    unsigned long dt = (time_queue_time() - *time);
    if(dt == 0) return 1;
    return ((size * 1000000 / dt) > (queue->quota));
}
/* Public: deletes the time queue structure and allocated content
 *
 * queue - the queue to delete
 */
void time_queue_delete(time_queue* queue)
{
    free(queue->values);
    free(queue->hashes);
    free(queue);
}
/*
 * Example:
int main(int argc, char** argv)
{
    time_queue* queue = time_queue_new(10, 42);
    time_queue_set(queue, "/var/log/lol");
    printf("%lu\n", *time_queue_get(queue, "/var/log/lol"));
    sleep(1);
    printf("%d\n", time_queue_overflows(queue, "/var/log/lol", 880));
    printf("%lu\n", time_queue_hash("hello world"));
    time_queue_delete(queue);
}
*/
