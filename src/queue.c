typedef struct _queue_event
{
    char* prefix;
    char* path;
    char* buf;
    size_t size;
    off_t offset;
    struct _queue_event* next;
} queue_event;
queue_event** event_last_get()
{
    static queue_event* last = NULL;
    return &last;
}
void event_enqueue(char *prefix, char *path, char *buf,
        size_t size, off_t offset)
{
    queue_event** last = event_last_get();
    queue_event* new = malloc(sizeof(queue_event));
    new->prefix = strdup(prefix);
    new->path = strdup(path);
    new->buf = strdup(buf);
    new->size = size;
    new->offset = offset;
    new->next = *last;
    *last = new;
}
void events_dequeue(void (*writer)(const char *prefix, const char *path, char *buf,
        size_t size, off_t offset))
{
    queue_event** last = event_last_get();
    while(*last != NULL)
    {
        writer((*last)->prefix, (*last)->path, (*last)->buf, (*last)->size, (*last)->offset);
        free((*last)->prefix);
        free((*last)->path);
        free((*last)->buf);
        queue_event* to_free = *last;
        *last = (*last)->next;
        free(to_free);
    }
}
