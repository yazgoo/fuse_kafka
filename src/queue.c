typedef struct _queue_event
{
    char* prefix;
    char* path;
    char* buf;
    size_t size;
    off_t offset;
    struct _queue_event* previous;
    struct _queue_event* next;
} queue_event;
queue_event** event_last_get()
{
    static queue_event* last = NULL;
    return &last;
}
int* event_queue_size()
{
    static int n = 0;
    return &n;
}
int* event_queue_max_size()
{
    static int n = 10000;
    return &n;
}
void events_drop_first()
{
    queue_event* current = *event_last_get();
    while(current != NULL)
    {
        if(current->previous != NULL && current->previous->previous == NULL)
        {
            free(current->previous);
            current->previous = NULL;
            (*event_queue_size())--;
            return;
        }
        current = current->previous;
    }
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
    new->previous = *last;
    if(new->previous != NULL) new->previous->next = new;
    new->next = NULL;
    *last = new;
    if((*event_queue_size())++ >= *(event_queue_max_size()))
        events_drop_first();
}
void event_delete(queue_event* event)
{
    free(event->prefix);
    free(event->path);
    free(event->buf);
    free(event);
    (*event_queue_size())--;
}
void events_queue_empty()
{
    queue_event** last = event_last_get();
    while(*last != NULL)
    {
        queue_event* to_free = *last;
        *last = (*last)->previous;
        event_delete(to_free);
    }
}
void events_dequeue(void (*writer)(const char *prefix, const char *path, char *buf,
        size_t size, off_t offset))
{
    queue_event* current = *event_last_get();
    if(current == NULL) return;
    while(current->previous != NULL) current = current->previous;
    for(; current != NULL; current = current->next)
    {
        writer(current->prefix, current->path,
                current->buf, current->size, current->offset);
    }
    events_queue_empty();
}
