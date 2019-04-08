#include "list.h"
#include <malloc.h>
struct genlist *list_new(void)
{
    struct genlist *ret =
        (struct genlist *) malloc(sizeof(struct genlist));
    if (!ret)
        return ret;
    ret->key = 0;
    ret->data = 0;
    ret->next = ret;
    ret->prev = ret;
    return ret;
}

struct genlist *list_lookup(struct genlist *l, long key)
{
    struct genlist *curr = l->next;
    while (curr != l && curr->key != key)
        curr = curr->next;
    if (curr == l)
        return 0;
    else
        return curr;
}

struct genlist *list_insert(struct genlist *l, long key, void *data)
{
    struct genlist *ret =
        (struct genlist *) malloc(sizeof(struct genlist));
    if (!ret)
        return ret;
    ret->key = key;
    ret->data = data;
    ret->next = l->next;
    ret->prev = l;
    l->next->prev = ret;
    l->next = ret;
    return ret;
}

void list_remove(struct genlist *l)
{
    l->next->prev = l->prev;
    l->prev->next = l->next;
    free(l);
}
