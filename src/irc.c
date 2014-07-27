#include "ircfunc.h"

void __circle_irc (IRC * irc)
{
    memset(irc, 0, sizeof(IRC));

    irc->init = &__irc_init;
    irc->shutdown = &__irc_shutdown;
    irc->respond = &__irc_respond;
    irc->kill = &__irc_kill;

    irc->log = &__irc_log;

    irc->__open_log = &__irc___open_log;
    irc->__close_log = &__irc___close_log;
    irc->__thread_loop = &__irc___thread_loop;
    irc->__process = &__irc___process;

    irc->parse= &__irc_parse;
    irc->get_directive = &__irc_get_directive;

    irc->get_nick = &__irc_get_nick;
    irc->get_target = &__irc_get_target;
    irc->get_kicked_nick = &__irc_get_kicked_nick;
}

int __irc_init (IRC * irc)
{
    pthread_t tid;
    int ret;
    
    ret = pthread_create(&tid, NULL, irc->__thread_loop, irc);
    if (ret) irc->__ircenv->log(irc->__ircenv, IRC_LOG_ERR, "Error in thread creation for id %d\n", irc->id);
    else irc->__ircenv->log(irc->__ircenv, IRC_LOG_NORM, "Child created at id %d\n", irc->id);
    irc->__pthread_irc = tid;
    return ret;
}

int __irc_shutdown (IRC * irc)
{
    irc->active = 0;
    irc->run = 0;
    irc->kill(irc);
    return pthread_join(irc->__pthread_irc, NULL);
}

void __irc_respond (IRC * irc, char * format, ...)
{
    char buffer[__CIRCLE_BUFF_SIZE_INITIAL+1];

    pthread_mutex_lock( &irc->__mutex );

    va_list listPointer;
    va_start( listPointer, format );
    
    memset(buffer, 0, __CIRCLE_BUFF_SIZE_INITIAL+1);
    vsnprintf(buffer, __CIRCLE_BUFF_SIZE_INITIAL-2, format, listPointer);
    buffer[strlen(buffer)] = '\r';
    buffer[strlen(buffer)] = '\n';
    irc->socket.write(&irc->socket, buffer);

    va_end( listPointer );
    usleep(__CIRCLE_WRITE_DELAY);

    pthread_mutex_unlock( &irc->__mutex );
}

int __irc_kill (IRC * irc)
{
    irc->respond(irc, "QUIT :Terminated!");
    return 0;
}

int __irc_log (IRC * irc, __irc_logtype type, const char * format, ...)
{
    va_list listPointer;
    FILE ** fptr, *std;
    time_t now;
    char buff[27];
    memset(buff, 0, 27);

    time(&now);
    ctime_r(&now, buff);
    buff[strlen(buff)-1] = '\0';

    va_start( listPointer, format );

    switch (type)
    {
        case IRC_LOG_ERR:
            fptr = &irc->__ircerr;
            std = stderr;
            break;
        case IRC_LOG_RAW:
            fptr = &irc->__ircraw;
            std = NULL;
            break;
        case IRC_LOG_NORM:
        default:
            fptr = &irc->__irclog;
            std = stdout;
            break;
    }

    if (irc->__ircenv->__ircargs.log)
    {
        if (type != IRC_LOG_RAW)
        {
            fprintf(*fptr, "[%s] [%d] ", buff, irc->id);
            vfprintf(*fptr, format, listPointer);
            fflush(*fptr);
        }
    }
    if (irc->__ircenv->__ircargs.raw)
    {
        if (type == IRC_LOG_RAW)
        {
            fprintf(*fptr, "%ld000 ", now);
            vfprintf(*fptr, format, listPointer);
            fflush(*fptr);
        }
    }
    if (irc->__ircenv->__ircargs.mode == IRC_MODE_NORMAL && std != NULL && type != IRC_LOG_RAW)
    {
        fprintf(std, "[%s] [%d] ", buff, irc->id);
        vfprintf(std, format, listPointer);
    }

    va_end( listPointer );

    return 0;
}

int __irc___open_log(IRC * irc, __irc_logtype type)
{
    char filename[__CIRCLE_LEN_FILENAME+1], *suffix;
    FILE ** fptr;
    memset(filename, 0, __CIRCLE_LEN_FILENAME+1);
    switch (type)
    {
        case IRC_LOG_ERR:
            suffix = "err.log";
            fptr = &irc->__ircerr;
            break;
        case IRC_LOG_RAW:
            suffix = "raw.log";
            fptr = &irc->__ircraw;
            break;
        case IRC_LOG_NORM:
        default:
            suffix = "log";
            fptr = &irc->__irclog;
            break;
    }
    snprintf(filename, __CIRCLE_LEN_FILENAME, "%s.%d.%s", irc->__ircenv->appname, irc->id, suffix);
    *fptr = fopen(filename, "a");
    if (*fptr == NULL) return errno;
    return 0;
}

int __irc___close_log(IRC * irc, __irc_logtype type)
{
    FILE ** fptr;
    int ret;
    switch (type)
    {
        case IRC_LOG_ERR:
            fptr = &irc->__ircerr;
            break;
        case IRC_LOG_RAW:
            fptr = &irc->__ircraw;
            break;
        case IRC_LOG_NORM:
        default:
            fptr = &irc->__irclog;
            break;
    }
    ret = fclose(*fptr);
    if (ret) return errno;
    return 0;
}

void * __irc___thread_loop (void * ptr)
{
    IRC * irc;
    IRCSOCK * s;
    IRCMSG ircmsg;
    int sleeptime, tmpsleep;
    __irc_line line;

    irc = (IRC *)(ptr);

    pthread_sigmask(SIG_BLOCK, &irc->__ircenv->__sigset, NULL);

    irc->run = 1;
    if (irc->__ircenv->__ircargs.log)
    {
        irc->__open_log(irc, IRC_LOG_NORM);
        irc->__open_log(irc, IRC_LOG_ERR);
    }
    
    sleeptime = 0;
    s = &irc->socket;
    __circle_ircsock(s);
    s->__irc = irc;

    irc->log(irc, IRC_LOG_NORM, "Thread started\n");
    while (irc->run)
    {
        if (sleeptime < __CIRCLE_SOCKET_CYCLE_MAX) sleeptime += __CIRCLE_SOCKET_CYCLE_INC;
        if (irc->enable)
        {
            if (strlen(s->host) == 0) strcpy(s->host, irc->host);
            if (s->port == 0) s->port = irc->port;
            irc->log(irc, IRC_LOG_NORM, "Connecting to %s:%d...\n", s->host, s->port);
            if (s->connect(s))
            {
                if (errno) irc->log(irc, IRC_LOG_ERR, "Error connecting to %s:%d: %s\n", s->host, s->port, strerror(errno));
                tmpsleep = sleeptime;
                while (tmpsleep--)
                {
                    sleep(1);
                    if (irc->run == 0) break;
                }
            }
            else
            {
                if (irc->__ircenv->__ircargs.raw) irc->__open_log(irc, IRC_LOG_RAW);
                irc->log(irc, IRC_LOG_NORM, "Connected; Logging in...\n");
                errno = 0;
                if (s->handshake(s, irc))
                {
                    if (errno == EPIPE)
                    {
                        s->disconnect(s);
                        sleep(sleeptime);
                        if (irc->__ircenv->__ircargs.raw) irc->__close_log(irc, IRC_LOG_RAW);
                        continue;
                    }
                    irc->log(irc, IRC_LOG_ERR, "Error in authentication\n");
                    s->disconnect(s);
                    if (irc->__ircenv->__ircargs.raw) irc->__close_log(irc, IRC_LOG_RAW);
                }
                else
                {
                    while (irc->active)
                    {
                        s->read(s, &line);
                        if (strlen(line.data) == 0) continue;
                        if (strstr(line.data, "PING :") != NULL)
                        {
                            s->write(s, "PONG :");
                            s->write(s, &line.data[6]);
                            s->write(s, "\r\n");
                        }
                        else
                        {
                            ircmsg = irc->parse(line.data);
                            irc->__process(irc, &ircmsg);
                        }
                    }
                    sleep(2);
                    s->disconnect(s);
                    if (irc->__ircenv->__ircargs.raw) irc->__close_log(irc, IRC_LOG_RAW);
                    if (irc->run == 0) break;
                    else irc->log(irc, IRC_LOG_ERR, "Socket closed for some reason; restarting\n");
                }
            }
        }
        else
        {
            tmpsleep = sleeptime;
            while (tmpsleep--)
            {
                sleep(1);
                if (irc->run == 0) break;
            }
        }
    }
    if (irc->__ircenv->__ircargs.log) irc->__close_log(irc, IRC_LOG_NORM);
    return NULL;
}

void __irc___process (IRC * irc, IRCMSG * ircmsg)
{
    int length;
    field_t ptarget, ftime;
    time_t ttime;
    char * servname, *msg;

    irc->log(irc, IRC_LOG_NORM, "<%s> <%s> <%s> <%s>\n", ircmsg->sender, ircmsg->command, ircmsg->target, ircmsg->message);

    if (!strcmp(ircmsg->command, "ERROR"))
    {
        irc->active = 0;
        irc->log(irc, IRC_LOG_NORM, "Connection closed: %s\n", ircmsg->message);
        return;
    }

    ptarget = irc->get_nick(ircmsg->sender);

    if (!strcmp(ircmsg->command, "001"))
    {
        servname = ircmsg->message + 15;
        length = strlen(servname);
        if (index(servname, ' ') != NULL) length = index(servname, ' ') - servname;
        if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
        strncpy(irc->name, servname, length);
        irc->name[length] = '\0';

        length = strlen(ircmsg->sender);
        if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
        strncpy(irc->socket.host, ircmsg->sender, length);
        irc->socket.host[length] = '\0';
    }
    else if (!strcmp(ircmsg->command, "NICK"))
    {
        if (strcasecmp(ptarget.data, irc->nickname) == 0)
            strncpy(irc->nickname, ircmsg->target, CIRCLE_FIELD_DEFAULT);
    }
    else if (!strcmp(ircmsg->command, "PRIVMSG"))
    {
        if (!strcmp(ircmsg->message, "\001VERSION\001"))
            irc->respond(irc, "NOTICE %s :\001VERSION %s written in C\001", ptarget.data, CIRCLE_VERSION);
        else if (!strcmp(ircmsg->message, "\001PING"))
            irc->respond(irc, "NOTICE %s :%s", ptarget.data, ircmsg->message);
        else if (!strcmp(ircmsg->message, "\001UPTIME\001"))
        {
            time(&ttime);
            ftime = __circle_time(ttime);
            irc->respond(irc, "NOTICE %s :%s", ptarget.data, ftime.data);
        }
        else if (!strncmp(ircmsg->message, CIRCLE_SENTINEL, strlen(CIRCLE_SENTINEL)))
        {
            msg = ircmsg->message + strlen(CIRCLE_SENTINEL);
            if (!strcmp(msg, "beep"))
                if (msg[4] == '\0' || msg[4] == ' ')
                    irc->respond(irc, "PRIVMSG %s :%cBEEP!%c", ircmsg->target, IRC_TXT_BOLD, IRC_BELL);
        }
    }

    ircmsg->irc = irc;
    irc->__ircenv->ircq.queue(&irc->__ircenv->ircq, *ircmsg);
}

IRCMSG __irc_parse (const char * line)
{
    IRCMSG ircmsg;
    memset(&ircmsg, 0, sizeof(IRCMSG));

    char *spa, *spb, *spc, *spm;
    int lna, lnb, lnc, lnt;

    spa = index(line, ' ');
    if (spa == NULL) return ircmsg;
    spa++;
    lna = (spa != NULL)?(spa - line - 1):0;
    spb = index(spa, ' ')+1;
    lnb = (spb != NULL)?(spb - spa - 1):0;
    if (line[0] == ':')
    {
        lnt = (lna-1 < CIRCLE_FIELD_SENDER)?lna-1:CIRCLE_FIELD_SENDER;
        strncpy(ircmsg.sender, line+1, lnt);

        lnt = (lnb < CIRCLE_FIELD_COMMAND)?lnb:CIRCLE_FIELD_COMMAND;
        strncpy(ircmsg.command, spa, lnt);
    }
    else
    {
        lnt = (lna < CIRCLE_FIELD_COMMAND)?lna:CIRCLE_FIELD_COMMAND;
        strncpy(ircmsg.command, line, lnt);
    }

    if (!strcmp(ircmsg.command, "JOIN") || !strcmp(ircmsg.command, "NICK"))
    {
        spb++;
        lnt = (strlen(spb) < CIRCLE_FIELD_TARGET)?strlen(spb):CIRCLE_FIELD_TARGET;
        strncpy(ircmsg.target, spb, lnt);
        return ircmsg;
    }
    else if (!strcmp(ircmsg.command, "QUIT"))
    {
        spb++;
        lnt = (strlen(spb) < CIRCLE_FIELD_MESSAGE)?strlen(spb):CIRCLE_FIELD_MESSAGE;
        strncpy(ircmsg.message, spb, lnt);
        return ircmsg;
    }
    else if (!strcmp(ircmsg.command, "PART"))
    {
        spc = index(line+2, ':');
        lnc = strlen(spb);
        if (spc != NULL)
        {
            lnc = (strlen(spc+1) < CIRCLE_FIELD_MESSAGE)?strlen(spc+1):CIRCLE_FIELD_MESSAGE;
            strncpy(ircmsg.message, spc+1, lnc);
            lnc = spc - spb - 1;
        }
        lnt = (lnc < CIRCLE_FIELD_TARGET)?lnc:CIRCLE_FIELD_TARGET;
        strncpy(ircmsg.target, spb, lnt);
        return ircmsg;
    }
    else
    {
        spc = index(spb, ' ')+1;
        lnc = (spc != NULL)?(spc - spb - 1):0;
        lnt = (lnc < CIRCLE_FIELD_TARGET)?lnc:CIRCLE_FIELD_TARGET;
        strncpy(ircmsg.target, spb, lnt);
    }

    int offset = 0;
    int extra = 0;
    if (strlen(ircmsg.sender) != 0)
    {
        offset += strlen(ircmsg.sender);
        extra++;
    }
    offset += strlen(ircmsg.command) + 1;
    if (strlen(ircmsg.target) != 0)
    {
        offset += strlen(ircmsg.target);
        extra++;
    }
    if (offset + extra < strlen(line) - 1)
    {
        spm = index(line + offset, ' ') + 1;
        if (spm[0] == ':') spm++;
        lnt = (strlen(spm) < CIRCLE_FIELD_MESSAGE)?strlen(spm):CIRCLE_FIELD_MESSAGE;
        strncpy(ircmsg.message, spm, lnt);
    }
    return ircmsg;
}

IRCCALL __irc_get_directive (const char * message)
{
    IRCCALL call;
    int i, length;
    const char * ptr;
    char *end;

    i = 0;
    memset(&call, 0, sizeof(IRCCALL));
    if (message == NULL) return call;

    if (strlen(message) < strlen(CIRCLE_SENTINEL)) return call;
    if (strncmp(message, CIRCLE_SENTINEL, strlen(CIRCLE_SENTINEL)) == 0)
    {
        ptr = &message[strlen(CIRCLE_SENTINEL)];
        end = index(ptr, ' ');
        if (end == NULL)
        {
            strncpy(call.command, ptr, CIRCLE_FIELD_MESSAGE);
            return call;
        }
        length = end - ptr;
        if (length > CIRCLE_FIELD_MESSAGE) length = CIRCLE_FIELD_MESSAGE;
        strncpy(call.command, ptr, length);
        if (end[0] == ' ') end++;
        if (end[0] == '\0') return call;
        strncpy(call.line, end, CIRCLE_FIELD_MESSAGE);

        char * aptr = call.line;
        char * naptr = index(call.line, ' ');
        while (naptr != NULL && i < CIRCLE_LEN_BOT_ARGS)
        {
            length = naptr-aptr;
            if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
            strncpy(call.arg[i].data, aptr, length);
            aptr = naptr+1;
            naptr = index(aptr, ' ');
            i++;
        }
        if (i < CIRCLE_LEN_BOT_ARGS)
        {
            length = strlen(aptr);
            if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
            strncpy(call.arg[i].data, aptr, length);
        }
    }
    return call;
}


field_t __irc_get_nick (const char * sender)
{
    field_t nick;
    int length;
    char * ptr;

    ptr = index(sender, '!');

    memset(&nick, 0, sizeof(field_t));
    if (ptr != NULL)
    {
        length = ptr - sender;
        if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
        strncpy(nick.data, sender, length);
    }
    return nick;
}

field_t __irc_get_target (const IRCMSG * ircmsg)
{
    field_t field;
    int length;

    memset(&field, 0, sizeof(field_t));
    if (strlen(ircmsg->target) > 0 && ircmsg->target[0] == '#')
    {
        length = strlen(ircmsg->target);
        if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
        strncpy(field.data, ircmsg->target, length);
    }
    else if (index(ircmsg->sender, '!') != NULL)
        field = ircmsg->irc->get_nick(ircmsg->sender);
    return field;
}

field_t __irc_get_kicked_nick (const char * message)
{
    char * ptr;
    field_t kicked;
    int length;

    ptr = index(message, ' ');

    memset(&kicked, 0, sizeof(field_t));
    if (ptr != NULL)
    {
        length = ptr - message;
        if (length > CIRCLE_FIELD_DEFAULT) length = CIRCLE_FIELD_DEFAULT;
        strncpy(kicked.data, message, length);
    }
    return kicked;
}
