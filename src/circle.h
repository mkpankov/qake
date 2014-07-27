/* 
 * File:   circle.h
 * Author: Sarah Harvey
 *
 * Created on June 5, 2010, 6:49 PM
 */

#ifndef _CIRCLE_H
#define	_CIRCLE_H

#ifdef	__cplusplus
extern "C" {
#endif

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <dlfcn.h>
#include <dirent.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <limits.h>
#include <ctype.h>
#include <getopt.h>

#ifdef CIRCLE_USE_DB
    #include <db.h>
#else
    #ifndef CIRCLE_USE_INTERNAL
    #define CIRCLE_USE_INTERNAL
    #endif
#endif

/******************************************************************************
 * Public Macro Definitions
 *****************************************************************************/

#define CIRCLE_DIR_MODULES      "modules"   /* module directory */
#define CIRCLE_DIR_LOGS         "logs"      /* log directory */

#define CIRCLE_NAME             "circle"    /* default name */
#define CIRCLE_INFO             "http://circle.sourceforge.net"
    /* Library information */
#define CIRCLE_COPYRIGHT        "Copyright (C) 2009-2010 Sarah Harvey"
    /* Copyright information */

#define CIRCLE_VERSION_MAJOR    0           /* major version number */
#define CIRCLE_VERSION_MINOR    8           /* minor version number */
#define CIRCLE_REVISION         144         /* revision number */
#define CIRCLE_VERSION_MODULE   1           /* module version number */
#define CIRCLE_VERSION          "circle 0.8"
    /* version string */

#define CIRCLE_SENTINEL         "!"         /* default command identifier */

#define CIRCLE_MODULE_EXT       ".so"       /* module extension */

#define CIRCLE_FIELD_DEFAULT    80          /* default field length */
#define CIRCLE_FIELD_SENDER     256         /* sender field length (msg_t) */
#define CIRCLE_FIELD_TARGET     64          /* target field length (msg_t) */
#define CIRCLE_FIELD_COMMAND    8           /* command field length (msg_t) */
#define CIRCLE_FIELD_MESSAGE    512         /* message field length (msg_t) */
#define CIRCLE_FIELD_ERROR      256         /* error field length */
#define CIRCLE_FIELD_FORMAT     384         /* default send buffer length */

#define CIRCLE_LEN_MAX_CHAN     8           /* max chans a bot can autojoin */
#define CIRCLE_LEN_BOT_ARGS     8           /* max args for bot commands */

/******************************************************************************
 * IRC Formatting Characters
 *****************************************************************************/

#define IRC_TXT_BOLD        '\x002'
#define IRC_TXT_ULIN        '\x015'
#define IRC_TXT_ITAL        '\x009'
#define IRC_TXT_COLR        '\x003'
#define IRC_TXT_NORM        '\x00F'

#define IRC_COL_WHITE       "00"
#define IRC_COL_BLACK       "01"
#define IRC_COL_BLUE        "02"
#define IRC_COL_GREEN       "03"
#define IRC_COL_RED         "04"
#define IRC_COL_BROWN       "05"
#define IRC_COL_PURPLE      "06"
#define IRC_COL_ORANGE      "07"
#define IRC_COL_YELLOW      "08"
#define IRC_COL_LTGRN       "09"
#define IRC_COL_TEAL        "10"
#define IRC_COL_CYAN        "11"
#define IRC_COL_LTBLU       "12"
#define IRC_COL_PINK        "13"
#define IRC_COL_GREY        "14"
#define IRC_COL_LTGRY       "15"

#define IRC_BELL            '\x007'

/******************************************************************************
 * Private Macro Definitions
 *****************************************************************************/

#define __CIRCLE_SOCKET_TIMEOUT         -1   /* socket timeout value */
#define __CIRCLE_SOCKET_CYCLE_MAX       120  /* max socket cycle time in s */
#define __CIRCLE_SOCKET_CYCLE_INC       10   /* socket cycle time increment */
#define __CIRCLE_WRITE_DELAY            200  /* delay after writes in ms */

#define __CIRCLE_BUFF_SIZE_INITIAL      128  /* initial buffer size */
#define __CIRCLE_BUFF_SIZE_INCREMENT    32   /* buffer increment amount */
#define __CIRCLE_BUFF_RECV_STATIC       1024 /* size of static buffer */

#define __CIRCLE_LEN_FILENAME           256  /* filename length */

#define __CIRCLE_LEN_LINE               256  /* line length */

/******************************************************************************
 * Initial Data Type Definitions
 *****************************************************************************/

typedef struct { char data[__CIRCLE_LEN_LINE+1]; } __irc_line;

typedef struct {
    int id;
    char user[CIRCLE_FIELD_SENDER + 1];
} __irc_auth;

typedef enum {
    IRC_LOG_NORM = 0,
    IRC_LOG_ERR = 1,
    IRC_LOG_RAW = 2
} __irc_logtype;            /* irc log type */

typedef enum {
    IRC_MODE_NORMAL = 0,
    IRC_MODE_DAEMON = 1,
    IRC_MODE_USAGE = 2,
    IRC_MODE_VERSION = 3
} __irc_mode;               /* irc mode */

struct __irc;       typedef struct __irc        IRC;
struct __ircmsg;    typedef struct __ircmsg     IRCMSG;
struct __irccall;   typedef struct __irccall    IRCCALL;
struct __ircmod;    typedef struct __ircmod     IRCMOD;
struct __ircenv;    typedef struct __ircenv     IRCENV;
struct __ircq;      typedef struct __ircq       IRCQ;
struct __ircsock;   typedef struct __ircsock    IRCSOCK;
struct __irchelp;   typedef struct __irchelp    IRCHELP;
struct __irchopt;   typedef struct __irchopt    IRCHOPT;

#ifdef CIRCLE_USE_INTERNAL
/* internal linked list type and function definitions */
struct __irclist;  typedef struct __irclist    IRCLIST;

struct __irclist {
    void * item;
    IRCLIST * next;
};

int     irclist_append  (IRCLIST ** first, void * item);
int     irclist_insert  (IRCLIST ** first, void * item, int location);
int     irclist_remove  (IRCLIST ** first, int location);
int     irclist_clear   (IRCLIST ** first);
void *  irclist_get     (IRCLIST ** first, int location);
void *  irclist_take    (IRCLIST ** first, int location);
int     irclist_size    (IRCLIST ** first);

int     irclist_get_max_irc_id (IRCLIST ** first);
int     irclist_get_irc_id (IRCLIST ** first, unsigned int id);
 #endif /* CIRCLE_USE_INTERNAL */

/******************************************************************************
 * Data Type Definitions
 *****************************************************************************/

typedef struct { char data[CIRCLE_FIELD_DEFAULT+1]; } field_t;
    /* field struct */
typedef struct { char data[CIRCLE_FIELD_ERROR+1]; } error_t;
    /* error struct */

struct __irchelp {
    int admin:1;
    char * command;
    char * usage;
    char * description;
    IRCHOPT * options;
    IRCHELP * next;
};

struct __irchopt {
    int admin:1;
    char * option;
    char * usage;
    char * description;
    unsigned int arguments;
    IRCHOPT * next;
};

struct __ircmsg {
    IRC * irc;
    char sender[CIRCLE_FIELD_SENDER+1];
    char target[CIRCLE_FIELD_TARGET+1];
    char command[CIRCLE_FIELD_COMMAND+1];
    char message[CIRCLE_FIELD_MESSAGE+1];
};

struct __irccall {
    char command[CIRCLE_FIELD_DEFAULT+1];
    field_t arg[CIRCLE_LEN_BOT_ARGS];

    char line[CIRCLE_FIELD_MESSAGE+1];
};

struct __ircmod {
    void * dlhandle;
    char filename[CIRCLE_FIELD_DEFAULT+1];
    void (*evaluate)(const IRCMSG * ircmsg);
    void (*construct) ();
    void (*destruct) ();
    IRCHELP * (*commands) ();
    int (*irc_version) ();
    char * (*name) ();
};

struct __ircsock {
    int (*connect) (IRCSOCK * sock);
    int (*disconnect) (IRCSOCK * sock);

    int (*handshake) (IRCSOCK * sock, IRC * irc);
    void (*identify) (IRCSOCK * sock, IRC * irc);
    void (*autojoin) (IRCSOCK * sock, IRC * irc);
    void (*quit) (IRCSOCK * sock, IRC * irc, char * message);

    int (*read) (IRCSOCK * sock, __irc_line * line);
    int (*write) (IRCSOCK * sock, char * line);
    int (*writef) (IRCSOCK * sock, char * format, ...);

    char host[CIRCLE_FIELD_DEFAULT+1];
    unsigned short int port;

    IRC * __irc;
    unsigned int __r;
    unsigned int __w;
    int __sockfd;
    char __buffer[__CIRCLE_BUFF_RECV_STATIC];
    int __pos;

    int (*__getc) (IRCSOCK * sock);
};

struct __irc {
    int (*init) (IRC * irc);
    int (*shutdown) (IRC * irc);
    void (*respond) (IRC * irc, char * format, ...);
    int (*kill) (IRC * irc);

    int (*log) (IRC * irc, __irc_logtype type, const char * message, ...);

    IRCMSG (*parse) (const char * raw);
    IRCCALL (*get_directive) (const char * message);

    field_t (*get_nick) (const char * sender);
    field_t (*get_target) (const IRCMSG * ircmsg);
    field_t (*get_kicked_nick) (const char * message);

    unsigned int id;
    unsigned int enable:1;
    volatile sig_atomic_t active;
    volatile sig_atomic_t run;
    IRCSOCK socket;

    char nickname[CIRCLE_FIELD_DEFAULT+1];
    char username[CIRCLE_FIELD_DEFAULT+1];
    char realname[CIRCLE_FIELD_DEFAULT+1];
    char password[CIRCLE_FIELD_DEFAULT+1];
    field_t channels[CIRCLE_LEN_MAX_CHAN];

    char admin[CIRCLE_FIELD_DEFAULT+1];

    char name[CIRCLE_FIELD_DEFAULT+1];
    char host[CIRCLE_FIELD_DEFAULT+1];
    unsigned short int port;

    IRCENV * __ircenv;
    FILE * __irclog;
    FILE * __ircraw;
    FILE * __ircerr;
    pthread_t __pthread_irc;
    pthread_mutex_t __mutex;

    int (*__open_log) (IRC * irc, __irc_logtype type);
    int (*__close_log) (IRC * irc, __irc_logtype type);
    void * (*__thread_loop) (void * ptr);
    void (*__process) (IRC * irc, IRCMSG * ircmsg);
};

typedef struct {
    char conf[__CIRCLE_LEN_FILENAME+1];
    unsigned int verbose:3;
    unsigned int log:1;
    unsigned int raw:1;
    __irc_mode mode;
} __args;                   /* struct holding circle arguments */

struct __ircq {
    int (*init) (IRCQ * ircq);
    int (*kill) (IRCQ * ircq);
    int (*queue) (IRCQ * ircq, IRCMSG ircmsg);
    int (*get_item) (IRCQ * ircq, IRCMSG * ircmsg);
    int (*clear) (IRCQ * ircq);

    int (*load) (IRCQ * ircq, const IRCMSG * ircmsg, char * file);
    int (*unload) (IRCQ * ircq, const IRCMSG * ircmsg, char * file);
    int (*reload) (IRCQ * ircq, const IRCMSG * ircmsg, char * file);
    int (*load_all) (IRCQ * ircq);
    int (*unload_all) (IRCQ * ircq);
    void (*commands) (IRCQ * ircq, const IRCMSG * ircmsg);
    void (*help) (IRCQ * ircq, const IRCMSG * ircmsg);
    void (*list) (IRCQ * ircq, const IRCMSG * ircmsg);
    void (*dir) (IRCQ * ircq, const IRCMSG * ircmsg);

    int (*log) (IRCQ * ircq, __irc_logtype type, const char * format, ...);

    int queue_num;

    pthread_t __pthread_q;
    IRCENV * __ircenv;
    IRCSOCK __socket;
    pthread_mutex_t __mutex;
    sigset_t __sigset;
    volatile sig_atomic_t active;

    #ifdef CIRCLE_USE_INTERNAL
    IRCLIST * __list_queue;
    IRCLIST * __list_modules;
    IRCLIST * __list_commands;
    #endif /* CIRCLE_USE_INTERNAL */
    #ifdef CIRCLE_USE_DB
    DB * __db_queue;
    DB * __db_modules;
    DB * __db_commands;
    #endif /* CIRCLE_USE_DB */

    void * (*__thread_loop) (void * ptr);
    void (*__eval) (IRCQ * ircq, const IRCMSG * ircmsg);
    void (*__process) (IRCQ * ircq, const IRCMSG * ircmsg);
    IRCHELP * (*__help_list) (IRCQ * ircq);
    int (*__empty) (IRCQ * ircq);
};

struct __ircenv {
    /* version and usage functions (terminates program) */
    void (*version) (void);
    void (*usage) (IRCENV * ircenv);

    /* starts bot */
    int (*init) (IRCENV * ircenv);
    int (*clean) (IRCENV * ircenv);

    /* configuration functions */
    int (*load_args) (IRCENV * ircenv, int argc, char ** argv);
    int (*load_config) (IRCENV * ircenv, const char * conf);
    int (*irc_create) (IRCENV * ircenv);
    int (*irc_destroy) (IRCENV * ircenv, int id);
    void (*irc_display) (IRCENV * ircenv, int id, const IRCMSG * ircmsg);
    void (*irc_display_all) (IRCENV * ircenv, const IRCMSG * ircmsg);

    /* administrative functions */
    int (*login) (IRCENV * ircenv, IRC * irc, const char * sender);
    int (*logout) (IRCENV * ircenv, IRC * irc, const char * nick);
    int (*auth) (IRCENV * ircenv, IRC * irc, const char * sender);
    int (*is_auth) (IRCENV * ircenv, IRC * irc, const char * sender);
    int (*deauth_all) (IRCENV * ircenv);

    int (*log) (IRCENV * ircenv, __irc_logtype type, const char * format, ...);

    char * appname;             /* application name (for usage function) */

    time_t time_start;          /* time of start */
    IRCQ ircq;                  /* IRC queue */

    pthread_t __pthread_main;
    volatile sig_atomic_t __active;
    IRC __default;
    error_t __error;
    sigset_t __sigset;
    __args __ircargs;
    FILE * __irclog;
    FILE * __ircerr;
    pthread_mutex_t __mutex_log;

    #ifdef CIRCLE_USE_INTERNAL
    IRCLIST * __list_irc;
    IRCLIST * __list_auth;
    #endif /* CIRCLE_USE_INTERNAL */
    #ifdef CIRCLE_USE_DB
    DB_ENV * dbenv;
    DB * __db_irc;
    DB * __db_auth;
    #endif /* CIRCLE_USE_DB */

    int (*__open_log) (IRCENV * ircenv, __irc_logtype type);
    int (*__close_log) (IRCENV * ircenv, __irc_logtype type);
    int (*__size) (IRCENV * ircenv);
    int (*__start_all) (IRCENV * ircenv);
    int (*__start) (IRCENV * ircenv, int id);
    int (*__kill_all) (IRCENV * ircenv);
};

/******************************************************************************
 * Public Function Definitions
 *****************************************************************************/

IRCENV circle_init(char * appname);

/* struct initializers */
void __circle_ircenv (IRCENV * ircenv);
void __circle_irc (IRC * irc);
void __circle_ircq (IRCQ * ircq);
void __circle_ircsock (IRCSOCK * ircsock);

field_t __circle_time(time_t time);

#ifdef	__cplusplus
}
#endif

#endif	/* _CIRCLE_H */

