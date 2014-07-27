#include "ircfunc.h"

void __circle_set_field(char * dest, char * src, int maxlen)
{
    int len = strlen(src);
    if (len > maxlen) len = maxlen;
    memset(dest, 0, maxlen+1);
    strncpy(dest, src, len);
}

field_t  __circle_time(time_t time)
{
    field_t result;
    int seconds, minutes, hours, days, years, left;
    char * buffer;

    seconds = time % 60;
    minutes = time / 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    days = hours / 24;
    hours = hours % 24;
    years = days / 365;
    days = days % 365;

    left = CIRCLE_FIELD_DEFAULT;
    buffer = result.data;
    memset(buffer, 0, CIRCLE_FIELD_DEFAULT+1);

    if (years) snprintf(buffer, left, "%dy, ", years);
    left -= strlen(buffer);
    buffer += strlen(buffer);
    if (days) snprintf(buffer, left, "%dd, ", days);
    left -= strlen(buffer);
    buffer += strlen(buffer);
    if (hours) snprintf(buffer, left, "%dh, ", hours);
    left -= strlen(buffer);
    buffer += strlen(buffer);
    if (minutes) snprintf(buffer, left, "%dm, ", minutes);
    left -= strlen(buffer);
    buffer += strlen(buffer);
    if (seconds) snprintf(buffer, left, "%ds, ", seconds);
    int len = strlen(buffer)-1;
    if (buffer[len] == ' ' || buffer[len] == ',') buffer[len] = '\0';
    len = strlen(buffer)-1;
    if (buffer[len] == ',') buffer[len] = '\0';
    if (strlen(buffer) == 0) snprintf(buffer, CIRCLE_FIELD_DEFAULT, "0s");

    return result;
}

void __circle_link_help(IRCHELP * list)
{
    int i, j;
    IRCHOPT * irchopt;

    for (i = 0; list[i].command != NULL; i++)
    {
        list[i].next = &list[i+1];
        if (list[i].options != NULL)
        {
            irchopt = list[i].options;
            for (j = 0; irchopt[j].option != NULL; j++) irchopt[j].next = &irchopt[j+1];
        }
    }
}

IRCHELP ** __circle_endptr_help(IRCHELP * list)
{
    int i;

    for (i = 0; list[i].command != NULL; i++);
    if (i == 0) return NULL;
    return &(list[i-1].next);
}

#ifdef CIRCLE_USE_INTERNAL
int     irclist_get_max_irc_id (IRCLIST ** first)
{
    IRCLIST * iterator;
    IRC * irc;
    unsigned int max;
    if (first == NULL) return -1;

    max = 0;
    iterator = *first;
    while (iterator != NULL)
    {
        irc = (IRC *)(iterator->item);
        if (irc->id > max) max = irc->id;
        iterator = iterator->next;
    }
    return max;
}

int     irclist_get_irc_id (IRCLIST ** first, unsigned int id)
{
    IRCLIST * iterator;
    IRC * irc;
    int i;
    if (first == NULL) return -1;

    i = 0;
    iterator = *first;
    while (iterator != NULL)
    {
        irc = (IRC *)(iterator->item);
        if (irc->id  == id) break;
        iterator = iterator->next;
        i++;
    }
    if (iterator == NULL) return -1;
    return i;
}
#endif /* CIRCLE_USE_INTERNAL */
