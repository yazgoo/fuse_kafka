#ifndef TIME_QUEUE_H
#define TIME_QUEUE_H
typedef struct
{
    unsigned long* values;
    unsigned long* hashes;
    unsigned int size, i, quota;
}
time_queue;
#endif
