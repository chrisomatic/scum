#include "headers.h"
#include "log.h"
#include "glist.h"

glist* list_create(void* buf, int max_count, int item_size, bool is_queue)
{
    if(item_size <= 0 || max_count <= 1)
    {
        LOGE("Invalid item_size (%d) or max_count (%d) for list", item_size, max_count);
        return NULL;
    }

    if(buf == NULL)
    {
        LOGE("List buffer is NULL");
        return NULL;
    }

    glist* list = calloc(1, sizeof(glist));

    list->count = 0;
    list->max_count = max_count;
    list->item_size = item_size;
    list->buf = buf;
    list->is_queue = is_queue;

    return list;
}

void list_print(glist* list)
{
    LOGI("[List (%s), %d/%d]:",list->is_queue ? "Queue" : "", list->count, list->max_count);

    for(int i = 0; i < list->count; ++i)
    {
        print_hex((uint8_t*)list->buf+(i*list->item_size), list->item_size);
    }
}

void list_delete(glist* list)
{
    if(list != NULL) free(list);
    list = NULL;
}

bool list_add(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(list_is_full(list))
    {
        if(!list->is_queue)
            return false;
        list->count--;
    }

    char* p = (char*)list->buf;

    if(list->is_queue)
    {
        memcpy(p+(list->item_size),p, list->count*list->item_size);
        memcpy(p, item, list->item_size);
    }
    else
    {
        memcpy(p+list->count*list->item_size, item, list->item_size);
    }

    list->count++;

    return true;
}

bool list_remove(glist* list, int index)
{
    if(list == NULL)
        return false;

    if(index >= list->count)
        return false;

    char* p = (char*)list->buf;

    if(list->is_queue)
    {
        memcpy(p+(index*list->item_size), p+((index+1)*list->item_size), (list->count-index-1)*list->item_size);
    }
    else
    {
        memcpy(p + index*list->item_size, p+(list->count-1)*list->item_size, list->item_size);
    }
    list->count--;
}

bool list_remove_by_item(glist* list, void* item)
{
    if(list == NULL)
        return false;

    if(item == NULL)
        return false;

    char* p = (char*)list->buf;

    if(!list->is_queue)
    {
        memcpy(item, p+(list->count-1)*list->item_size, list->item_size);
        list->count--;
    }
    else
    {
        int index = -1;
        for(int i = 0; i < list->count; ++i)
        {
            if(list->buf+(i*list->item_size) == item)
            {
                return list_remove(list, i);
            }
        }
    }
}

bool list_clear(glist* list)
{
    if(list == NULL)
        return false;
    list->count = 0;
}

bool list_is_full(glist* list)
{
    return (list->count >= list->max_count);
}

bool list_is_empty(glist* list)
{
    return (list->count == 0);
}

void* list_get(glist* list, int index)
{
    if(list == NULL)
        return NULL;

    char* p = (char*)list->buf;

    return (void*)(p + index*list->item_size);
}

void list_test()
{
    int buffer[10] = {0};

    glist* list = list_create(buffer, 10, sizeof(int), true);

    int x;
    x = 1;
    list_add(list, &x);
    x = 2;
    list_add(list, &x);
    x = 3;
    list_add(list, &x);
    x = 4;
    list_add(list, &x);

    // remove 3
    list_remove(list, 1);

    x = 5;
    list_add(list, &x);
    
    // remove 4
    list_remove(list, 1);

    // 5
    // 2
    // 1
    list_print(list);


    // test filling up
    x = 9;
    list_add(list, &x);
    list_add(list, &x);
    list_add(list, &x);
    list_add(list, &x);
    list_add(list, &x);
    list_add(list, &x);
    list_add(list, &x);

    x = 8;
    list_add(list, &x); // should remove oldest 

    list_print(list);

    list_delete(list);
}
