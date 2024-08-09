typedef struct
{
    int max_count;
    int count;
    size_t item_size;
    void* buf;
} CircBuf;

void circbuf_create(CircBuf* cb, int max_count, size_t item_size)
{
    cb->item_size = item_size;
    cb->count = 0;
    cb->max_count = max_count;
    cb->buf = malloc(item_size*max_count);
}

void circbuf_clear_items(CircBuf* cb)
{
    cb->count = 0;
}

void circbuf_delete(CircBuf* cb)
{
    if(cb->buf)
    {
        free(cb->buf);
        cb->buf = NULL;
    }
    cb->count = 0;
}

void circbuf_add(CircBuf* cb, void* item)
{
    int index = cb->count;

    unsigned char* p = (unsigned char*)cb->buf;

    if (cb->count == cb->max_count)
    {
        // shift
        for (int i = 1; i <= cb->max_count - 1; ++i)
        {
            unsigned char* p1 = p + (i*cb->item_size);
            unsigned char* p2 = p + ((i-1)*cb->item_size);
            memcpy(p2, p1, cb->item_size);
        }

        index--;
    }
    else
    {
        cb->count++;
    }

    void* it = (p+(index*cb->item_size));
    memcpy(it,item,cb->item_size);
}

void circbuf_print(CircBuf* cb)
{
    printf("CircBuf (%p) [Item Count: %d, Item Size: %d]:\n", cb, cb->count, cb->item_size);

    for(int i = 0; i < cb->count; ++i)
    {
        printf(" Item %d: [", i);
        unsigned char* p = cb->buf + (i*cb->item_size);

        for(int j = 0; j < cb->item_size; ++j)
        {
            printf(" %02X", p[j]);
        }
        printf(" ]\n");
    }

    printf("\n");
}

void* circbuf_get_item(CircBuf* cb,int index)
{
    if(index < 0 || index >= cb->max_count)
        return NULL;

    unsigned char* p = (unsigned char*)cb->buf;
    return (p+(index*cb->item_size));
}
