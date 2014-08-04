#include "ircfunc.h"

void __circle_ircsock (IRCSOCK * ircsock)
{
    memset(ircsock, 0, sizeof(IRCSOCK));

    ircsock->connect = &__ircsock_connect;
    ircsock->disconnect = &__ircsock_disconnect;

    ircsock->handshake = &__ircsock_handshake;
    ircsock->identify = &__ircsock_identify;
    ircsock->autojoin = &__ircsock_autojoin;
    //ircsock->quit = &__ircsock_quit;

    ircsock->read = &__ircsock_read;
    ircsock->write = &__ircsock_write;
    //ircsock->writef = &__ircsock_writef;
    ircsock->__getc = &__ircsock___getc;

    ircsock->__sockfd = -1;
}

int __ircsock_connect (IRCSOCK * sock)
{
    struct addrinfo hints;
    struct addrinfo *result, *res_ptr;
    int ai_res;
    char sport[6];
    IRC * irc;

    irc = sock->__irc;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(sport, 6, "%d", sock->port);

    switch((ai_res = getaddrinfo(sock->host, sport, &hints, &result)))
    {
        case 0: break;
        default:
            if (irc) irc->log(irc, IRC_LOG_ERR, "getaddrinfo failed: %s\n", gai_strerror(ai_res));
            return -1;
            break;
    }

    res_ptr = result;
    do
    {
        if ((sock->__sockfd = socket(res_ptr->ai_family, res_ptr->ai_socktype, res_ptr->ai_protocol)) != -1)
        {
            if (connect(sock->__sockfd, res_ptr->ai_addr, res_ptr->ai_addrlen) == 0) break;
            close(sock->__sockfd);
            sock->__sockfd = -1;
        }

    } while ((res_ptr = res_ptr->ai_next) != NULL);

    freeaddrinfo(result);
    if (res_ptr == NULL)
    {
        if (irc) irc->log(irc, IRC_LOG_ERR, "Connect failed\n", irc->id);
        return -1;
    }
    return 0;
}

int __ircsock_disconnect (IRCSOCK * sock)
{
    close(sock->__sockfd);
    sock->__sockfd = -1;
    return 0;
}

int __ircsock_handshake (IRCSOCK * sock, IRC * irc)
{
    char nickbuff[CIRCLE_FIELD_DEFAULT+1];
    IRCMSG ircmsg;
    int rand, npos;
    unsigned int seed;
    __irc_line line;

    sleep(1);

    memset(nickbuff, 0, CIRCLE_FIELD_DEFAULT+1);

    sock->write(sock, "\r\n");
    sock->write(sock, "NICK ");
    sock->write(sock, irc->nickname);
    sock->write(sock, "\r\n");
    if (errno)
    {
        irc->log(irc, IRC_LOG_ERR, "Error writing to stream: %s\n", strerror(errno));
        sock->disconnect(sock);
        return errno;
    }
    sock->write(sock, "USER ");
    sock->write(sock, irc->username);
    sock->write(sock, " * * :");
    sock->write(sock, irc->realname);
    sock->write(sock, "\r\n");
    if (errno)
    {
        irc->log(irc, IRC_LOG_ERR, "Error writing to stream: %s\n", strerror(errno));
        sock->disconnect(sock);
        return errno;
    }

    while (1)
    {
        if (sock->read(sock, &line))
        {
            irc->log(irc, IRC_LOG_ERR, "Error reading from socket\n");
            sock->disconnect(sock);
            return -1;
        }
        else if (strstr(line.data, " NOTICE AUTH :") != NULL) continue;
        else if (strstr(line.data, "PING :") != NULL)
        {
            sock->write(sock, "PONG :");
            sock->write(sock, &line.data[6]);
            sock->write(sock, "\r\n");
            continue;
        }
        else if (strstr(line.data, " 433 * ") != NULL)
        {
            irc->log(irc, IRC_LOG_ERR, "Nick collision detected!\n");
            seed = time(NULL);
            rand = rand_r(&seed) % 9999;

            npos = CIRCLE_FIELD_DEFAULT - 4;
            if ((CIRCLE_FIELD_DEFAULT - strlen(irc->nickname)) > 4)
                npos = strlen(irc->nickname);

            memset(nickbuff, 0, CIRCLE_FIELD_DEFAULT+1);
            memcpy(nickbuff, irc->nickname, npos);
            snprintf(nickbuff+npos, 5, "%04d", rand);

            sock->write(sock, "\r\n");
            sock->write(sock, "NICK ");
            sock->write(sock, nickbuff);
            sock->write(sock, "\r\n");

            if (errno)
            {
                irc->log(irc, IRC_LOG_ERR, "Error writing to stream: %s\n", strerror(errno));
                sock->disconnect(sock);
                return -1;
            }
            sock->write(sock, "USER ");
            sock->write(sock, irc->username);
            sock->write(sock, " * * :");
            sock->write(sock, irc->realname);
            sock->write(sock, "\r\n");

            if (errno)
            {
                irc->log(irc, IRC_LOG_ERR, "Error writing to stream: %s\n", strerror(errno));
                sock->disconnect(sock);
                return -1;
            }
        }
        else
        {
            irc->active = 1;
            if (strlen(nickbuff) > 0) memcpy(irc->nickname, nickbuff, CIRCLE_FIELD_DEFAULT+1);
            if (strlen(irc->password) > 0) sock->identify(sock, irc);
            if (strlen(irc->channels[0].data) > 0) sock->autojoin(sock, irc);
            ircmsg = irc->parse(line.data);
            irc->__process(irc, &ircmsg);
            break;
        }
    }
    return 0;
}

void __ircsock_identify (IRCSOCK * sock, IRC * irc)
{
    sock->write(sock, "NICKSERV IDENTIFY ");
    sock->write(sock, irc->password);
    sock->write(sock, "\r\n");
}

void __ircsock_autojoin (IRCSOCK * sock, IRC * irc)
{
    int i;

    for (i = 0; i < CIRCLE_LEN_MAX_CHAN; i++)
    {
        if (strlen(irc->channels[i].data))
        {
            sock->write(sock, "JOIN ");
            sock->write(sock, irc->channels[i].data);
            sock->write(sock, "\r\n");
        }
    }
}

void __ircsock_quit (IRCSOCK * sock, IRC * irc, char * message);

int __ircsock_read (IRCSOCK * sock, __irc_line * line)
{
    int count;
    char c, * buffer;
    IRC * irc;
    
    irc = sock->__irc;

    buffer = line->data;

    c = EOF;
    count = 0;
    while ((c = sock->__getc(sock)) != EOF && c != '\n') buffer[count++] = c;

    if (errno) return errno;
    
    buffer[count] = '\0';
    if (count > 0 && buffer[count-1] == '\r') buffer[count-1] = '\0';
    if (c == EOF && strlen(buffer) == 0) return EOF;

    irc->log(irc, IRC_LOG_RAW, "%s\n", buffer);

    return 0;
}

int __ircsock_write (IRCSOCK * sock, char * line)
{
    ssize_t written, tmp;
    IRC * irc;

    irc = sock->__irc;
    written = 0;

    if (sock->__sockfd == -1) return -1;
    
    while (written < strlen(line))
    {
        tmp = write(sock->__sockfd, &line[written], strlen(line) - written);
        if (tmp == -1) return -1;
        written += tmp;
    }

    return 0;
}

int __ircsock_writef (IRCSOCK * sock, char * format, ...);

int __ircsock___getc (IRCSOCK * sock)
{
    char * fbuff;
    int frsize;
    
    fbuff = sock->__buffer;

    if (fbuff[sock->__pos] == '\0')
    {
        frsize = 0;
        if (!(frsize = read(sock->__sockfd, fbuff, __CIRCLE_BUFF_SIZE_INITIAL)) || errno)
            return EOF;
        fbuff[frsize] = '\0';
        sock->__pos = 0;

        sock->__r += frsize;
    }
    return fbuff[sock->__pos++];
}
