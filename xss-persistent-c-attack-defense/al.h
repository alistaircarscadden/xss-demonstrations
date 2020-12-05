/**
 * Header Only Array List Implementation
 * Alistair Carscadden <alistaircarscadden@gmail.com>
 * 
 * Compile with -DCDS_STRING_ARRAY_LIST to get helpful functions for
 * string lists.
*/


#ifndef __CDS_ARRAYLIST__
#define __CDS_ARRAYLIST__

#include <stdlib.h>

#ifdef CDS_STRING_ARRAY_LIST
#include <string.h>
#endif

typedef struct
{
    int capacity;
    int length;
    void **data;
} array_list;

#ifdef CDS_STRING_ARRAY_LIST
int alist_find_s(array_list *al, const char *p);
int alist_remove_s(array_list *al, const char *p);
#endif

int alist_find(array_list *al, const void *p);
int alist_create(array_list *al, int capacity);
int alist_destroy(array_list *al);
int alist_push(array_list *al, void *p);
int alist_remove_i(array_list *al, int index);
int alist_remove(array_list *al, const void *p);
void alist_clear(array_list *al);
void *alist_get(array_list *al, int index);

int alist_create(array_list *al, int capacity)
{
    if (!al)
        return 1;

    al->length = 0;
    al->capacity = capacity;
    al->data = calloc(capacity, sizeof(void *));

    if (!al->data)
        return 1;

    return 0;
}

int alist_destroy(array_list *al) {
    if (!al)
        return 1;

    al->length = 0;
    al->capacity = 0;
    free(al->data);
    al->data = NULL;

    return 0;
}

int alist_push(array_list *al, void *p)
{
    if (!al)
        return 1;

    if (al->length == al->capacity)
    {
        int new_capacity = al->capacity * 2;
        al->data = realloc(al->data, sizeof(void *) * new_capacity);
        if (!al->data)
            return 1;
        al->capacity = new_capacity;
    }

    al->data[al->length] = p;
    al->length++;

    return 0;
}

int alist_remove_i(array_list *al, int index)
{
    if (index < 0 || index >= al->length)
        return 1;

    for (int i = index; i < al->length; i++)
    {
        al->data[i] = al->data[i + 1];
    }
    al->length--;

    return 0;
}

int alist_find(array_list *al, const void *p)
{
    for (int i = 0; i < al->length; i++)
    {
        if (al->data[i] == p)
        {
            return i;
        }
    }
    return -1;
}

int alist_remove(array_list *al, const void *p)
{
    int index = alist_find(al, p);

    if (index == -1)
        return 1;

    return alist_remove_i(al, index);
}

void alist_clear(array_list *al)
{
    al->length = 0;
}

void *alist_get(array_list *al, int index)
{
    return al->data[index];
}

#ifdef CDS_STRING_ARRAY_LIST
int alist_remove_s(array_list *al, const char *p)
{
    int index = alist_find_s(al, p);

    if (index == -1)
        return 1;

    return alist_remove_i(al, index);
}


int alist_find_s(array_list *al, const char *p)
{
    for (int i = 0; i < al->length; i++)
    {
        if (strcmp((char *)al->data[i], p) == 0)
        {
            return i;
        }
    }
    return -1;
}
#endif

#endif