#include "ircfunc.h"

void __circle_ircq (IRCQ * ircq)
{
    memset(ircq, 0, sizeof(IRCQ));

    ircq->init = &__ircq_init;
    ircq->kill = &__ircq_kill;
    ircq->dir = &__ircq_dir;
    ircq->log = &__ircq_log;

    ircq->commands = &__ircq_commands;
    ircq->help = &__ircq_help;

    #ifdef CIRCLE_USE_INTERNAL
    ircq->queue = &__ircq_queue_irclist;
    ircq->get_item = &__ircq_get_item_irclist;
    ircq->clear = &__ircq_clear_irclist;

    ircq->load = &__ircq_load_irclist;
    ircq->unload = &__ircq_unload_irclist;
    ircq->reload = &__ircq_reload_irclist;
    ircq->load_all = &__ircq_load_all_irclist;
    ircq->unload_all = &__ircq_unload_all_irclist;
    ircq->list = &__ircq_list_irclist;
    ircq->__process = &__ircq___process_irclist;
    ircq->__help_list = &__ircq___help_list_irclist;
    ircq->__empty = &__ircq___empty_irclist;
    #endif /* CIRCLE_USE_INTERNAL */

    #ifdef CIRCLE_USE_DB
    ircq->queue = &__ircq_queue_db;
    ircq->get_item = &__ircq_get_item_db;
    ircq->clear = &__ircq_clear_db;

    ircq->load = &__ircq_load_db;
    ircq->unload = &__ircq_unload_db;
    ircq->reload = &__ircq_reload_db;
    ircq->load_all = &__ircq_load_all_db;
    ircq->unload_all = &__ircq_unload_all_db;
    ircq->commands = &__ircq_commands_db;
    ircq->list = &__ircq_list_db;
    ircq->__process = &__ircq___process_db;
    ircq->__help_list = &__ircq___help_list_db;
    ircq->__empty = &__ircq___empty_db;
    #endif /* CIRCLE_USE_DB */

    ircq->__thread_loop = &__ircq___thread_loop;
    ircq->__eval = &__ircq___eval;
}

int __ircq_init (IRCQ * ircq)
{
    int ret;
    pthread_mutexattr_t mattr;

    pthread_mutexattr_init(&mattr);
    pthread_mutex_init(&ircq->__mutex, &mattr);

    ircq->active = 1;
    ret = pthread_create(&ircq->__pthread_q, NULL, ircq->__thread_loop, ircq);
    if (ret) ircq->log(ircq, IRC_LOG_ERR, "Creating the library thread returned error code %d\n", ret);
    else ircq->log(ircq, IRC_LOG_NORM, "Module thread created successfully\n");
    return ret;
}

int __ircq_kill (IRCQ * ircq)
{
    ircq->active = 0;
    pthread_kill(ircq->__pthread_q, SIGUSR1);
    pthread_kill(ircq->__pthread_q, SIGUSR1);
    return pthread_join(ircq->__pthread_q, NULL);
}

int __ircq_log (IRCQ * ircq, __irc_logtype type, const char * format, ...)
{
    pthread_mutexattr_t attr;
    va_list listPointer;
    FILE ** fptr, *std;

    time_t now;
    char buff[27];
    memset(buff, 0, 27);

    time(&now);
    ctime_r(&now, buff);
    buff[strlen(buff)-1] = '\0';

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&ircq->__ircenv->__mutex_log, &attr);
    pthread_mutex_lock( &ircq->__ircenv->__mutex_log );

    va_start( listPointer, format );

    switch (type)
    {
        case IRC_LOG_ERR:
            fptr = &ircq->__ircenv->__ircerr;
            std = stderr;
            break;
        case IRC_LOG_NORM:
        default:
            fptr = &ircq->__ircenv->__irclog;
            std = stdout;
            break;
    }

    if (ircq->__ircenv->__ircargs.log)
    {
        fprintf(*fptr, "[%s] [Q] ", buff);
        vfprintf(*fptr, format, listPointer);
        fflush(*fptr);
    }
    if (ircq->__ircenv->__ircargs.mode == IRC_MODE_NORMAL)
    {
        fprintf(std, "[%s] [Q] ", buff);
        vfprintf(std, format, listPointer);
    }

    va_end( listPointer );
    pthread_mutex_unlock( &ircq->__ircenv->__mutex_log );

    return 0;
}

void * __ircq___thread_loop (void * ptr)
{
    IRCQ * ircq = (IRCQ *)(ptr);
    int signal, res;
    IRCMSG ircmsg;
    
    pthread_sigmask(SIG_BLOCK, &ircq->__ircenv->__sigset, NULL);
    
    if (ircq->load_all(ircq)) ircq->log(ircq, IRC_LOG_ERR, "Error loading modules\n");

    sigemptyset(&ircq->__sigset);
    sigaddset(&ircq->__sigset, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &ircq->__sigset, NULL);
    signal = 0;
    while (ircq->active)
    {
        sigwait(&ircq->__sigset, &signal);
        if (signal == SIGUSR1)
        {
            pthread_mutex_lock( &ircq->__mutex );
            if (!ircq->__ircenv->__active) ircq->clear(ircq);
            else
            {
                while (!ircq->__empty(ircq))
                {
                    res = ircq->get_item(ircq, &ircmsg);
                    if (!res)
                    {
                        ircq->__eval(ircq, &ircmsg);
                        ircq->__process(ircq, &ircmsg);
                    }
                }
            }
            pthread_mutex_unlock( &ircq->__mutex );
        }
    }

    ircq->clear(ircq);
    ircq->log(ircq, IRC_LOG_NORM, "Queue cleared: %d hiding\n", ircq->queue_num);

    if (ircq->unload_all(ircq)) ircq->log(ircq, IRC_LOG_ERR, "Error unloading modules\n");

    return NULL;
}

void __ircq_help (IRCQ * ircq, const IRCMSG * ircmsg)
{
    IRCHELP * iterator;
    IRCHOPT * optrator;
    IRC * irc;
    IRCCALL irccall;
    field_t target, nick;
    
    irc = ircmsg->irc;
    irccall = irc->get_directive(ircmsg->message);

    if (strcmp(irccall.command, "help")) return;

    nick = irc->get_nick(ircmsg->sender);
    target = irc->get_target(ircmsg);

    if (!irccall.arg[0].data[0])
    {
        irc->respond(irc, "PRIVMSG %s :%s (%s) at your service! For a list of commands, type %c%scommands%c.", target.data, irc->nickname, CIRCLE_NAME, IRC_TXT_BOLD, CIRCLE_SENTINEL, IRC_TXT_NORM);
        return;
    }

    iterator = ircq->__help_list(ircq);
    while (iterator->command != NULL)
    {
        if (strcmp(iterator->command, irccall.arg[0].data))
        {
            iterator = iterator->next;
            continue;
        }
        if (irccall.arg[1].data[0])
        {
            optrator = iterator->options;
            while (optrator->option != NULL)
            {
                if (strcmp(optrator->option, irccall.arg[1].data))
                {
                    optrator = optrator->next;
                    continue;
                }
                irc->respond(irc, "PRIVMSG %s :%s - %c%s%c Syntax: %s (%s)", target.data, nick.data,
                        IRC_TXT_BOLD, irccall.arg[0].data, IRC_TXT_NORM,
                        optrator->usage, optrator->description);
                return;
            }
            if (optrator->option == NULL)
            {
                irc->respond(irc, "PRIVMSG %s :%s - command %c%s%c does not have option %c%s%c", target.data, nick.data,
                        IRC_TXT_BOLD, irccall.arg[0].data, IRC_TXT_NORM,
                        IRC_TXT_BOLD, irccall.arg[1].data, IRC_TXT_NORM);
            }
        }
        else
        {
            irc->respond(irc, "PRIVMSG %s :%s - %c%s%c Syntax: %s (%s)", target.data, nick.data,
                    IRC_TXT_BOLD, irccall.arg[0].data, IRC_TXT_NORM,
                    iterator->usage, iterator->description);
        }

        return;
    }
    irc->respond(irc, "PRIVMSG %s :%s - no help for command %c%s%c", target.data, nick.data,
        IRC_TXT_BOLD, irccall.arg[0].data, IRC_TXT_NORM);
}

void __ircq_commands (IRCQ * ircq, const IRCMSG * ircmsg)
{
    IRCHELP * iterator;
    IRC * irc;
    char buff[CIRCLE_FIELD_FORMAT+1];
    int pos;
    field_t target, nick;

    irc = ircmsg->irc;
    memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
    pos = 0;
    target = irc->get_target(ircmsg);
    nick = irc->get_nick(ircmsg->sender);

    irc->respond(irc, "PRIVMSG %s :%s - List of %cCommands%c:", target.data, nick.data, IRC_TXT_BOLD, IRC_TXT_NORM);
    iterator = ircq->__help_list(ircq);

    while (iterator->command != NULL)
    {
        if (strlen(iterator->command) > (CIRCLE_FIELD_FORMAT - pos - 2))
        {
            irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
            memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
            pos = 0;
        }
        if (strlen(buff))
        {
            strcpy(buff+pos, ", ");
            pos += 2;
        }
        strcpy(buff+pos, iterator->command);
        pos += strlen(iterator->command);
        iterator = iterator->next;
    }

    if (strlen(buff) > 0) irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
}

void __ircq___eval (IRCQ * ircq, const IRCMSG * ircmsg)
{
    IRCCALL irccall;
    IRC * irc;
    field_t target, nick, temp;
    time_t now;
    FILE * file;
    char buff[3][CIRCLE_FIELD_DEFAULT+1], *str;

    irc = ircmsg->irc;

    irccall = irc->get_directive(ircmsg->message);
    target = irc->get_target(ircmsg);
    nick = irc->get_nick(ircmsg->sender);
    
    if (strlen(irccall.command) > 0)
    {
        if (!strcmp(irccall.command, "help"))
            ircq->help(ircq, ircmsg);
        else if (!strcmp(irccall.command, "commands"))
            ircq->commands(ircq, ircmsg);
        else if (!strcmp(irccall.command, "admin"))
        {
            if (!strcmp(irccall.arg[0].data, "login"))
            {
                if (!strcmp(irccall.arg[1].data, irc->admin))
                    if (!ircq->__ircenv->auth(ircq->__ircenv, irc, ircmsg->sender))
                        irc->respond(irc, "PRIVMSG %s :%s has authenticated as \"%s\"", target.data, nick.data, ircmsg->sender);
                    else
                        irc->respond(irc, "PRIVMSG %s :%s - There was a problem authenticating; perhaps you already logged in?", target.data, nick.data);
                else
                    irc->respond(irc, "PRIVMSG %s :%s - Invalid password", target.data, nick.data);
            }
        }
        else if (!strcmp(irccall.command, "info"))
        {
            file = fopen("/proc/loadavg", "r");
            if (file == NULL) irc->respond(irc, "PRIVMSG %s :%s - Error opening /proc/avg for status: %s", target.data, nick.data, strerror(errno));
            else
            {
                memset(buff[0], 0, CIRCLE_FIELD_DEFAULT+1);
                str = fgets(buff[0], CIRCLE_FIELD_DEFAULT, file);
                fclose(file);
                if (str == NULL)
                    irc->respond(irc, "PRIVMSG %s :%s - Error reading /proc/avg for status: %s", target.data, nick.data, strerror(errno));
                else
                {
                    file = fopen("/proc/self/statm", "r");
                    if (file == NULL) irc->respond(irc, "PRIVMSG %s :%s - Error opening /proc/self/statm for status: %s", target.data, nick.data, strerror(errno));
                    else
                    {
                        memset(buff[1], 0, CIRCLE_FIELD_DEFAULT+1);
                        str = fgets(buff[1], CIRCLE_FIELD_DEFAULT, file);
                        fclose(file);
                        if (str == NULL)
                            irc->respond(irc, "PRIVMSG %s :%s - Error reading /proc/self/statm for status: %s", target.data, nick.data, strerror(errno));
                        else
                        {
                            buff[0][14] = '\0';
                            char * loadavg = buff[0];
                            char * vss_s = buff[1];
                            char * rss_s = index(buff[1], ' ');
                            rss_s[0] = '\0';
                            rss_s++;
                            char * end = index(rss_s, ' ');
                            end[0] = '\0';
                            int pagesize = sysconf(_SC_PAGESIZE);
                            int vss = atoi(vss_s);
                            int rss = atoi(rss_s);
                            irc->respond(irc, "PRIVMSG %s :%cLoad Avg:%c %s; %cVSZ:%c %dkB; %cRSS:%c %dkB; %cThreadNum:%c %d", target.data, IRC_TXT_BOLD, IRC_TXT_NORM, loadavg, IRC_TXT_BOLD, IRC_TXT_NORM, vss*pagesize/1024, IRC_TXT_BOLD, IRC_TXT_NORM, rss*pagesize/1024, IRC_TXT_BOLD, IRC_TXT_NORM, ircq->__ircenv->__size(ircq->__ircenv));
                        }
                    }
                }
            }
        }
        else if (!strcmp(irccall.command, "uptime"))
        {
            time(&now);
            temp = __circle_time(now - ircq->__ircenv->time_start);
            irc->respond(irc, "PRIVMSG %s :%s - %cUptime:%c %s", target.data, nick.data, IRC_TXT_BOLD, IRC_TXT_NORM, temp.data);
        }
        else if (!strcmp(irccall.command, "version"))
        {
            irc->respond(irc, "PRIVMSG %s :%s written in C (%s)", target.data, CIRCLE_VERSION, CIRCLE_INFO);
        }
        else if (!strcmp(irccall.command, "raw"))
        {
            if (ircq->__ircenv->is_auth(ircq->__ircenv, irc, ircmsg->sender))
                irc->respond(irc, "%s", irccall.line);
            else
                irc->respond(irc, "PRIVMSG %s :%s - You are not logged in", target.data, nick.data);
        }
        else if (!strcmp(irccall.command, "network"))
        {
            if (!strcmp(irccall.arg[0].data, "display"))
            {
                char * val = irccall.arg[1].data;
                int id = atoi(val);
                if (id <= 0) irc->respond(irc, "PRIVMSG %s :%s - Invalid id!\n", target.data, nick.data);
                else ircq->__ircenv->irc_display(ircq->__ircenv, id, ircmsg);
            }
            else if (!strcmp(irccall.arg[0].data, "list"))
                ircq->__ircenv->irc_display_all(ircq->__ircenv, ircmsg);
            else
                irc->respond(irc, "PRIVMSG %s :Invalid network command", target.data);
        }
        else if (!strcmp(irccall.command, "module"))
        {
            if (!strcmp(irccall.arg[0].data, "dir"))
            {
                ircq->dir(ircq, ircmsg);
            }
            else if (!strcmp(irccall.arg[0].data, "list"))
            {
                ircq->list(ircq, ircmsg);
            }
            else if (!strcmp(irccall.arg[0].data, "load"))
            {
                if (ircq->__ircenv->is_auth(ircq->__ircenv, irc, ircmsg->sender))
                    ircq->load(ircq, ircmsg, irccall.arg[1].data);
                else
                    irc->respond(irc, "PRIVMSG %s :%s - You are not logged in", target.data, nick.data);
            }
            else if (!strcmp(irccall.arg[0].data, "unload"))
            {
                if (ircq->__ircenv->is_auth(ircq->__ircenv, irc, ircmsg->sender))
                    ircq->unload(ircq, ircmsg, irccall.arg[1].data);
                else
                    irc->respond(irc, "PRIVMSG %s :%s - You are not logged in", target.data, nick.data);
            }
            else if (!strcmp(irccall.arg[0].data, "reload"))
            {
                if (ircq->__ircenv->is_auth(ircq->__ircenv, irc, ircmsg->sender))
                    ircq->reload(ircq, ircmsg, irccall.arg[1].data);
                else
                    irc->respond(irc, "PRIVMSG %s :%s - You are not logged in", target.data, nick.data);
            }
            else
                irc->respond(irc, "PRIVMSG %s :Invalid module command", target.data);
        }

    }
}

void __ircq_dir (IRCQ * ircq, const IRCMSG * ircmsg)
{
    DIR * dir;
    struct dirent * dir_entry;
    char *ext, buff[CIRCLE_FIELD_FORMAT+1], * str;
    int res, pos;
    IRC * irc;
    field_t target;

    irc = ircmsg->irc;
    target = irc->get_target(ircmsg);

    dir = opendir(CIRCLE_DIR_MODULES);
    if (dir == NULL)
    {
        errno = 0;
        irc->respond(irc, "PRIVMSG %s :Error opening %c%s%c for listing", target.data, IRC_TXT_BOLD, CIRCLE_DIR_MODULES, IRC_TXT_NORM);
        return;
    }

    pos = 0;

    irc->respond(irc, "PRIVMSG %s :Listing modules directory (%c%s%c):", target.data, IRC_TXT_BOLD, CIRCLE_DIR_MODULES, IRC_TXT_NORM);

    res = 0;
    memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
    while ((dir_entry = readdir(dir)) != NULL)
    {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) continue;
        ext = rindex(dir_entry->d_name, '.');
        if (ext == NULL) continue;
        if (strcmp(ext, CIRCLE_MODULE_EXT) != 0) continue;

        str = dir_entry->d_name;
        if (strlen(str) > (CIRCLE_FIELD_FORMAT - pos - 2))
        {
            irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
            memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
            pos = 0;
        }
        if (strlen(buff) > 0)
        {
            strcpy(buff+pos, ", ");
            pos += 2;
        }
        strcpy(buff+pos, str);
        pos += strlen(str);

        res++;
    }
    closedir(dir);
    if (errno) errno = 0;

    if ((strlen(buff) > 0) && ++res) irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
    if (!res) irc->respond(irc, "PRIVMSG %s :Nothing in directory", target.data);
}

#ifdef CIRCLE_USE_INTERNAL

int __ircq_queue_irclist (IRCQ * ircq, IRCMSG ircmsg)
{
    IRCMSG * q;
    int ret, result;

    pthread_mutex_lock( &ircq->__mutex );

    q = malloc(sizeof(IRCMSG));
    if (q == NULL) result = -1;
    else
    {
        memcpy(q, &ircmsg, sizeof(IRCMSG));
        ret = irclist_append(&ircq->__list_queue, q);
        if (ret)
        {
            free(q);
            result = -1;
        }
        else result = 0;
    }

    if (!result) ircq->queue_num++;

    pthread_mutex_unlock( &ircq->__mutex );
    pthread_kill(ircq->__pthread_q, SIGUSR1);
    return result;
}

int __ircq_clear_irclist (IRCQ * ircq)
{
    irclist_clear(&ircq->__list_queue);
    return 0;
}

int __ircq_get_item_irclist(IRCQ * ircq, IRCMSG * ircmsg)
{
    IRCMSG * im;
    int ret;

    im = (IRCMSG *)(irclist_take(&ircq->__list_queue, 0));
    if (im == NULL) ret = -1;
    else
    {
        memcpy(ircmsg, im, sizeof(IRCMSG));
        free(im);
        ret = 0;
        ircq->queue_num--;
    }
    return ret;
}

int __ircq_load_irclist (IRCQ * ircq, const IRCMSG * ircmsg, char * file)
{
    void * mhandle;
    IRC * irc;
    IRCLIST * iterator;
    IRCMOD * mod;
    char mfile[__CIRCLE_LEN_FILENAME+1], *error;
    field_t nick, target;
    
    iterator = ircq->__list_modules;
    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        if (strcmp(file, mod->filename) == 0)
        {
            if (ircmsg != NULL)
            {
                irc = ircmsg->irc;
                target = irc->get_target(ircmsg);
                nick = irc->get_nick(ircmsg->sender);
                irc->respond(irc, "PRIVMSG %s :%s - Error loading %c%s%c: file already loaded", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM);
            }
            return -1;
        }
        iterator = iterator->next;
    }

    memset(mfile, 0, __CIRCLE_LEN_FILENAME+1);
    snprintf(mfile, __CIRCLE_LEN_FILENAME, "%s/%s", CIRCLE_DIR_MODULES, file);

    mhandle = dlopen(mfile, RTLD_NOW);
    if (!mhandle)
    {
        error = dlerror();
        if (ircmsg != NULL)
        {
            irc = ircmsg->irc;
            target = irc->get_target(ircmsg);
            nick = irc->get_nick(ircmsg->sender);
            irc->respond(irc, "PRIVMSG %s :%s - Error opening %c%s%c: %s", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM, error);
        }

        dlerror();
        return -1;
    }

    mod = malloc(sizeof(IRCMOD));
    memset(mod, 0, sizeof(IRCMOD));
    strncpy(mod->filename, file, CIRCLE_FIELD_DEFAULT);

    mod->evaluate = dlsym(mhandle, "evaluate");

    if ((error = dlerror()))
    {
        if (ircmsg != NULL)
        {
            irc = ircmsg->irc;
            target = irc->get_target(ircmsg);
            nick = irc->get_nick(ircmsg->sender);
            irc->respond(irc, "PRIVMSG %s :%s - Error binding %c%s%c: %s", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM, error);
        }
        dlerror();
        free(mod);
        return -1;
    }

    mod->irc_version = dlsym(mhandle, "irc_version");
    error = NULL;
    if ((error = dlerror()) || (mod->irc_version() != CIRCLE_VERSION_MODULE))
    {
        if (ircmsg != NULL)
        {
            irc = ircmsg->irc;
            target = irc->get_target(ircmsg);
            nick = irc->get_nick(ircmsg->sender);
            if (!error) error = "Invalid module version";
            irc->respond(irc, "PRIVMSG %s :%s - Error binding %c%s%c: %s", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM, error);

        }
        dlerror();
        free(mod);
        return -1;
    }

    mod->construct = dlsym(mhandle, "construct");
    if (dlerror())
    {
        dlerror();
        mod->construct = NULL;
    }

    mod->destruct = dlsym(mhandle, "destruct");
    if (dlerror())
    {
        dlerror();
        mod->destruct = NULL;
    }

    mod->commands = dlsym(mhandle, "commands");
    if (dlerror())
    {
        dlerror();
        mod->commands = NULL;
    }

    mod->name = dlsym(mhandle, "name");
    if (dlerror())
    {
        dlerror();
        mod->name = NULL;
    }

    mod->dlhandle = mhandle;
    irclist_append(&ircq->__list_modules, mod);

    if (ircmsg != NULL)
    {
        irc = ircmsg->irc;
        target = irc->get_target(ircmsg);
        nick = irc->get_nick(ircmsg->sender);
        irc->respond(irc, "PRIVMSG %s :%s - Successfully loaded %c%s%c", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM);
    }

    if (mod->construct) mod->construct();

    return 0;
}

int __ircq_unload_irclist (IRCQ * ircq, const IRCMSG * ircmsg, char * file)
{
    IRC * irc;
    IRCLIST * iterator;
    IRCMOD * mod;
    int i;
    field_t nick, target;

    i = 0;
    iterator = ircq->__list_modules;
    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        if (strcmp(file, mod->filename) == 0) break;
        i++;
        iterator = iterator->next;
    }

    if (iterator == NULL)
    {
        if (ircmsg != NULL)
        {
            irc = ircmsg->irc;
            target = irc->get_target(ircmsg);
            nick = irc->get_nick(ircmsg->sender);
            irc->respond(irc, "PRIVMSG %s :%s - Error unloading %c%s%c: module does not exist", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM);
        }
        return 0;
    }
    else
    {
        mod = (IRCMOD *)(irclist_take(&ircq->__list_modules, i));
        if (mod->destruct) mod->destruct();
        dlclose(mod->dlhandle);

        if (ircmsg != NULL)
        {
            irc = ircmsg->irc;
            target = irc->get_target(ircmsg);
            nick = irc->get_nick(ircmsg->sender);
            irc->respond(irc, "PRIVMSG %s :%s - Successfully unloaded %c%s%c", target.data, nick.data, IRC_TXT_BOLD, file, IRC_TXT_NORM);
        }
        free(mod);
        return 0;
    }
}

int __ircq_reload_irclist (IRCQ * ircq, const IRCMSG * ircmsg, char * file)
{
    int res;
    res = ircq->unload(ircq, ircmsg, file);
    if (res) return res;
    return ircq->load(ircq, ircmsg, file);
}

int __ircq_load_all_irclist (IRCQ * ircq)
{
    DIR * dir;
    struct dirent * dir_entry;
    char *ext;
    int res, r;

    dir = opendir(CIRCLE_DIR_MODULES);
    if (dir == NULL)
    {
        errno = 0;
        return 1;
    }

    res = 0;
    while ((dir_entry = readdir(dir)) != NULL)
    {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) continue;
        ext = rindex(dir_entry->d_name, '.');
        if (ext == NULL) continue;
        if (strcmp(ext, CIRCLE_MODULE_EXT) != 0) continue;
        if ((r = ircq->load(ircq, NULL, dir_entry->d_name))) res = r;
    }
    closedir(dir);
    if (errno) errno = 0;
    return res;
}

int __ircq_unload_all_irclist (IRCQ * ircq)
{
    IRCLIST * iterator;
    IRCMOD * mod;
    int res, r;

    res = 0;
    iterator = ircq->__list_modules;
    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        iterator = iterator->next;
        if ((r = ircq->unload(ircq, NULL, mod->filename))) res = r;
    }
    return res;
}

void __ircq_list_irclist (IRCQ * ircq, const IRCMSG * ircmsg)
{
    IRCLIST * iterator;
    IRC * irc;
    IRCMOD * mod;
    char buff[CIRCLE_FIELD_FORMAT+1], buff2[CIRCLE_FIELD_DEFAULT+1];
    int pos, res;
    field_t target;

    irc = ircmsg->irc;
    iterator = ircq->__list_modules;
    memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
    pos = 0;
    res = 0;
    target = irc->get_target(ircmsg);
    irc->respond(irc, "PRIVMSG %s :Listing all loaded modules:", target.data);

    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        memset(buff2, 0, CIRCLE_FIELD_DEFAULT+1);
        if (strlen(mod->name()) == 0)
            snprintf(buff2, CIRCLE_FIELD_DEFAULT, "%c%s%c", IRC_TXT_BOLD, mod->filename, IRC_TXT_NORM);
        else
            snprintf(buff2, CIRCLE_FIELD_DEFAULT, "%s (%c%s%c)", mod->name(), IRC_TXT_BOLD, mod->filename, IRC_TXT_NORM);
        if (strlen(buff2) > (CIRCLE_FIELD_FORMAT - pos - 2))
        {
            irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
            memset(buff, 0, CIRCLE_FIELD_FORMAT+1);
            pos = 0;
            res++;
        }
        if (strlen(buff) > 0)
        {
            strcpy(buff+pos, ", ");
            pos += 2;
        }
        strcpy(buff+pos, buff2);
        pos += strlen(buff2);
        iterator = iterator->next;
    }

    if ((strlen(buff) > 0) && ++res) irc->respond(irc, "PRIVMSG %s :%s", target.data, buff);
    if (!res) irc->respond(irc, "PRIVMSG %s :No modules loaded", target.data);
}

IRCHELP * __ircq___help_list_irclist (IRCQ * ircq)
{
    IRCLIST * iterator;
    IRCHELP * imodhelp, **helpptr;
    IRCMOD * mod;

    static IRCHOPT netopt[] = {
                {0, "display", "network display [id]", "Displays network information", 1, 0},
                {0, "list", "network list", "Lists all connected networks", 0, 0},
                /*{1, "add", "network add", "Adds a network", 0, 0},
                {1, "remove", "network remove [id]", "Removes a network", 1, 0},
                {1, "set", "network set [id] [param]=[value]", "Sets network parameters", 2, 0},*/
                {0, 0, 0, 0, 0}
            };
    static IRCHOPT modopt[] = {
                {1, "load", "module load [file]", "Tries to load a module into the process", 1, 0},
                {1, "unload", "module unload [file]", "Tries to unload a module", 1, 0},
                {0, "list", "module list", "Lists all loaded modules", 0, 0},
                {0, "dir", "module dir", "Lists all modules in module directory", 0, 0},
                {0, 0, 0, 0, 0, 0}
            };

    static IRCHOPT adminopt[] = {
                {1, "login", "admin login [password]", "Authenticates user", 1, 0},
                /*{1, "logout", "admin logout [password]", "Deauthenticates user", 1, 0},*/
                {0, "list", "admin list", "Lists all authenticated users", 0, 0},
                {0, 0, 0, 0, 0, 0}
            };

    static IRCHELP help[] = {
        {0, "help",     "help [option]", "Displays detailed help on all commands", 0, 0},
        {0, "commands", "commands", "Displays all the commands", 0, 0},
        {0, "admin",    "admin [option] [argument]", "Administrative interface", adminopt, 0},
        {0, "info",     "info", "Display information on the running process", 0, 0},
        {0, "version",  "version", "Displays the version of the bot", 0, 0},
        {0, "uptime",   "uptime", "Displays uptime information", 0, 0},
        {1, "raw",      "raw [IRC protocol data]", "Sends data directly to the IRC network", 0, 0},
        {0, "network",  "network [option] [arguments..]", "Network information/configuration", netopt, 0},
        {0, "module",   "module [option] [arguments..]", "Module system configuration", modopt, 0},
        {0, "beep",     "beep", "Plays system bell", 0, 0},
        {0, 0, 0, 0, 0, 0}
    };

    __circle_link_help(help);
    helpptr = __circle_endptr_help(help);

    iterator = ircq->__list_modules;
    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        if (mod->commands == NULL)
        {
            iterator = iterator->next;
            continue;
        }
        imodhelp = mod->commands();
        if (imodhelp == NULL)
        {
            iterator = iterator->next;
            continue;
        }
        if (imodhelp[0].command == NULL)
        {
            iterator = iterator->next;
            continue;
        }
        __circle_link_help(imodhelp);
        *helpptr = imodhelp;
        helpptr = __circle_endptr_help(imodhelp);
        iterator = iterator->next;
    }

    return help;
}

void __ircq___process_irclist (IRCQ * ircq, const IRCMSG * ircmsg)
{
    IRCLIST * iterator;
    IRCMOD * mod;

    iterator = ircq->__list_modules;
    while (iterator != NULL)
    {
        mod = (IRCMOD *)(iterator->item);
        mod->evaluate(ircmsg);
        iterator = iterator->next;
    }
}

int __ircq___empty_irclist (IRCQ * ircq)
{
    return irclist_size(&ircq->__list_queue) == 0;
}

#endif /* CIRCLE_USE_INTERNAL */
