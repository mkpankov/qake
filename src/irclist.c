#include "ircfunc.h"
#ifdef CIRCLE_USE_INTERNAL

int     irclist_append  (IRCLIST ** first, void * item)
{
    IRCLIST * node, *iterator;
    if (first == NULL) return -1;
    node = malloc(sizeof(IRCLIST));
    memset(node, 0, sizeof(IRCLIST));
    if (node == NULL) return -1;
    node->item = item;
    if (*first == NULL) *first = node;
    else
    {
        iterator = *first;
        while (iterator->next != NULL) iterator = iterator->next;
        iterator->next = node;
    }
    return 0;
}

int     irclist_insert  (IRCLIST ** first, void * item, int location)
{
    IRCLIST * node, *n_iterator, *p_iterator;
    int i;
    if (first == NULL) return -1;
    if (location > irclist_size(first) || location < 0) return -1;
    node = malloc(sizeof(IRCLIST));
    memset(node, 0, sizeof(IRCLIST));
    if (node == NULL) return -1;
    node->item = item;
    if (location == 0)
    {
        node->next = *first;
        *first = node;
    }
    else
    {
        i = 1;
        n_iterator = (*first)->next;
        p_iterator = *first;
        while (i < location)
        {
            p_iterator = n_iterator;
            n_iterator = n_iterator->next;
            i++;
        }
        p_iterator->next = node;
        node->next = n_iterator;
    }
    return 0;
}

int     irclist_remove  (IRCLIST ** first, int location)
{
    IRCLIST * node, *tmp, * n_iterator, * p_iterator;
    int i;
    if (first == NULL) return -1;
    if (location > irclist_size(first)-1 || location < 0) return -1;
    if (location == 0)
    {
        node = (*first)->next;
        tmp = *first;
        *first = node;
        free(tmp->item);
        free(tmp);
    }
    else
    {
        i = 1;
        n_iterator = (*first)->next;
        p_iterator = *first;
        while (i < location)
        {
            p_iterator = n_iterator;
            n_iterator = n_iterator->next;
            i++;
        }
        tmp = n_iterator;
        p_iterator->next = n_iterator->next;
        free(tmp->item);
        free(tmp);
    }
    return 0;
}

int     irclist_clear   (IRCLIST ** first)
{
    IRCLIST *tmp, * iterator;
    if (first == NULL) return -1;
    if (*first == NULL) return 0;
    iterator = *first;
    *first = NULL;
    tmp = iterator;
    while (iterator != NULL)
    {
        tmp = iterator;
        iterator = iterator->next;
        free(tmp->item);
        free(tmp);
    }
    return 0;
}

void *  irclist_get     (IRCLIST ** first, int location)
{
    IRCLIST * iterator;
    int i;
    if (first == NULL) return NULL;
    if (*first == NULL) return NULL;
    if (location > irclist_size(first)-1 || location < 0) return NULL;

    i = 0;
    iterator  = *first;
    while (i < location)
    {
        iterator = iterator->next;
        i++;
    }
    return iterator->item;
}

void *  irclist_take    (IRCLIST ** first, int location)
{
    IRCLIST * node, *tmp, * n_iterator, * p_iterator;
    void * item;
    int i;
    if (first == NULL) return NULL;
    if (location > irclist_size(first)-1 || location < 0) return NULL;
    if (location == 0)
    {
        node = (*first)->next;
        tmp = *first;
        *first = node;
        item = tmp->item;
        free(tmp);
        return item;
    }
    else
    {
        i = 1;
        n_iterator = (*first)->next;
        p_iterator = *first;
        while (i < location)
        {
            p_iterator = n_iterator;
            n_iterator = n_iterator->next;
            i++;
        }
        tmp = n_iterator;
        p_iterator->next = n_iterator->next;
        item = tmp->item;
        free(tmp);
        return item;
    }
}

int     irclist_size    (IRCLIST ** first)
{
    IRCLIST * iterator;
    int i;
    if (first == NULL) return -1;

    i = 0;
    iterator  = *first;
    while (iterator != NULL)
    {
        iterator = iterator->next;
        i++;
    }
    return i;
}

#endif /* CIRCLE_USE_INTERNAL */
