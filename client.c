#include "client.h"

me_t me;

void client_help() {
    printf("HELP:\n");
    printf("  /help or /h      print this message\n");
    printf("  /list or /l      get list of users\n");
    printf("  /prevmsg or /pr  get previous messages\n");
    printf("  /clear or /clr   clear the screen\n");
    printf("  /quit or /q      clear the screen\n");
    printf("  /rn <name>       rename yourself\n");
}

void recv_users(int clifd) {
    uint32_t buf_size;
    int status = recv(clifd, &buf_size, sizeof(uint32_t), 0);
    DEBUG("UL_SIZE: %d", buf_size);

    int s1;
    char *buf = malloc(buf_size);
    status = recv(clifd, buf, buf_size, 0);
    if (status < 0) return;
    printf("\033[F\033[2K\r"); // go up one line and clear it
    printf("List of users: ");
    for (int i = 0; i < buf_size; i++) {
        if (buf[i] != 0)
            printf("%c", buf[i]);
        else
            printf("; ");
    }
    printf("\n");
    free(buf);
}
bool check_user_rename(char *buf) {
    if (strlen(buf) < 7 || strlen(buf) >= MAX_USERNAME_LEN+4) return false;
    if (buf[0] != '/' || buf[1] != 'r' || buf[2] != 'n') return false;
    return true;
}

void send_user_rename(me_t *me) {
    me->buf += 4;
    strcpy(me->username, me->buf);
    write(me->clifd, me->buf, strlen(me->buf));
}

void recv_user_rename(int clifd) {
    char buf[MAX_USERNAME_LEN*2+2];
    int status = read(clifd, buf, MAX_USERNAME_LEN*2+2);
    printf("'%s' got renamed to '%s'\n", buf, buf+strlen(buf));
}

void recv_disconnected(int clifd) {
    int status;
    char username[MAX_USERNAME_LEN];
    status = read(clifd, username, MAX_USERNAME_LEN);
    printf("\033[2K\r"); // go up one line and clear it
    printf(CNR"User "CBW"'%s'"CNR" disconnected"CCLR"\n", username);
}

void recv_connected(int clifd) {
    int status;
    char username[MAX_USERNAME_LEN];
    status = read(clifd, username, MAX_USERNAME_LEN);
    printf("\033[2K\r"); // go up one line and clear it
    printf(CNG"User "CBW"'%s'"CNG" connected"CCLR"\n", username);
}

void recv_msg_list(int clifd) {
    int status;
    int msg_list_size;
    char msg_buf[MAX_BUF_LEN];
    char username_buf[MAX_BUF_LEN];

    status = read(clifd, &msg_list_size, sizeof(int));
    if (msg_list_size <= 0) {
        printf("No messages sent yet or old messages disabled");
        return;
    }
    if (status < 0) {
        ERROR("Couldn't read msg_list_size (status: %d)", status);
        return;
    }
    DEBUG("msg_list size: %d", msg_list_size);

    char *buf = malloc(msg_list_size);
    int total = 0;
    status = 0;
    while (total < msg_list_size && status >= 0) {
        status = read(clifd, buf+total, msg_list_size-total);
        if (status < 0) return;
        total += status;
    }
    int bp = 0, mo = 0;
    int nr = 0;

    mo = strlen(buf+bp)+1;
    if (mo + bp+strlen(buf+bp+mo)+1 >= msg_list_size
            && *(buf+bp) == 'C' && *(buf+bp+1) == 0
            && strcmp(me.username, buf+bp+mo) == 0) {
        // printf("\033[F\033[2K\r"); // go up one line and clear line
        printf("\033[2K\r"); // clear line
        DEBUG("Your own login msg");
        bp += mo + strlen(buf+bp+mo)+1;
    } else {
        printf("\033[2K\r"); // clear line
        printf(CBC"    --- OLD MESSAGES ---    \n"CCLR);
    }
    while (bp < msg_list_size) {
        mo = strlen(buf+bp)+1;

        // skip last msg if it's your own login message
        // printf("%d %lu ", bp, bp + mo + strlen(buf+bp+mo)+1);
        if (mo + bp+strlen(buf+bp+mo)+1 >= msg_list_size
                && *(buf+bp) == 'C' && *(buf+bp+1) == 0
                && strcmp(me.username, buf+bp+mo) == 0) {
            DEBUG("Your own login msg");
            break;
        }

        if (*(buf+bp) == 'C' && *(buf+bp+1) == 0) {
            // printf("User '%s' connected\n", buf+bp+mo);
            printf(CNG"User "CBW"'%s'"CNG" connected"CCLR"\n", buf+bp+mo);
        } else if (*(buf+bp) == 'D' && *(buf+bp+1) == 0) {
            printf(CNR"User "CBW"'%s'"CNR" disconnected"CCLR"\n", buf+bp+mo);
            // printf("User '%s' disconnected\n", buf+bp+mo);
        } else {
            printf(CBW"(%s)"CCLR": %s\n", buf+bp, buf+bp+mo);
        }

        nr ++;
        bp += mo + strlen(buf+bp+mo)+1;
    }

    if (nr > 0)
        printf(CBC"--- RECIEVED %d MESSAGES ---\n"CCLR, nr);
    
    free(buf);
    // printf("\033[2K\r"); // go up one line and clear it
    // printf("MSG: %s: %s\n", username_buf, msg_buf);
}

void *send_loop(void *arg) {
    // struct me_t me = *(struct me_t *)arg;
    // char buf[MAX_BUF_LEN];

    DEBUG("Getting previously sent messages");
    {
        char id = P_MSG_LIST;
        int status = write(me.clifd, &id, 1);
        if (status < 0) ERROR("Couldn't request msg_list");
    }

    int pos = 0, status = 0;
    while (status >= 0) {
        memset(me.buf, 0, MAX_BUF_LEN);
        printf("\033[2K\r"); // go up one line and clear it
        printf(" "CBC"%s"CCLR" > ", me.username);
        pos = 0;

        while (pos < MAX_BUF_LEN) {
            me.buf[pos] = getc(stdin);
            me.buf[pos+1] = 0;
            if (pos > 0 && me.buf[pos] == '\n'){
                break;
            } else if (me.buf[pos] == '\n') {
                printf(" "CBC"%s"CCLR" > ", me.username);
            } else {
                pos++;
            }
        }
        me.buf[pos] = 0;

        char id = P_MSG;
        if (strcmp("/help", me.buf) == 0 || strcmp("/h", me.buf) == 0) {
            client_help();
            continue;
        } else if (strcmp("/list", me.buf) == 0 || strcmp("/l", me.buf) == 0) {
            id = P_USER_LIST;
        } else if (strcmp("/prevmsg", me.buf) == 0 || strcmp("/pr", me.buf) == 0) {
            id = P_MSG_LIST;
        } else if (check_user_rename(me.buf)) {
            id = P_USER_RENAME;
        } else if (strcmp("/clear", me.buf) == 0 || strcmp("/clr", me.buf) == 0) {
            printf("\033[2J\033[H");
            continue;
        } else if (strcmp("/quit", me.buf) == 0 || strcmp("/q", me.buf) == 0) {
            exit(0);
        } else if (me.buf[0] == '/') {
            printf("Type /help for help\n");
            continue;
        }

        status = send(me.clifd, &id, sizeof(id), 0);
        if (status < 0) return 0;

        switch (id) {
        case P_MSG:
            printf("\033[F\033[2K\r"); // go up one line and clear it
            printf(CBC"(%s)"CCLR": %s\n", me.username, me.buf);
            DEBUG("(%lu) Sending: %s", strlen(me.buf), me.buf);
            status = send(me.clifd, me.buf, strlen(me.buf), 0);
            break;
        // case P_USER_LIST:
        //     DEBUG("Recieving users");
        //     break;
        // case P_MSG_LIST:
        //     DEBUG("Recieving previous messages");
        //     break;
        case P_USER_RENAME:
            // DEBUG("sending rename");
            send_user_rename(&me);
            break;
        default:
            DEBUG("not catched %d", id);
            break;
        }
    }
    return 0;
}

int client(char username[MAX_USERNAME_LEN], int port, char *host) {
    INFO("Creating client socket");
    int clifd = socket(AF_INET, SOCK_STREAM, 0);
    if (clifd < 0) {
        ERROR("Can't create client socket");
        return 0;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        ERROR("Can't convert host.");
        return 0;
    }

    INFO("Connecting to server");
    if (connect(clifd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERROR("Can't connect to host!");
        return 0;
    }
    
    DEBUG("Sending username (%lu) '%s'", strlen(username), username);
    int status = send(clifd, username, strlen(username)+1, 0);
    if (status < 0) {
        ERROR("Couldn't send username!");
    }

    pthread_t thread_id;

    char buf[MAX_BUF_LEN];
    me.clifd = clifd;
    me.buf = buf;
    me.username = username;
    pthread_create(&thread_id, NULL, send_loop, &me);

    char msg_buf[MAX_BUF_LEN];
    char username_buf[MAX_USERNAME_LEN];

    printf("Type /help for help\n");
    char type;
    while (status > 0) {
        status = read(clifd, &type, 1);
        if (status < 0) break;
        switch (type) {
            case P_MSG:
                memset(username_buf, 0, MAX_USERNAME_LEN);
                status = read(clifd, username_buf, MAX_USERNAME_LEN);
                if (status < 0) return 0;
                memset(msg_buf, 0, MAX_BUF_LEN);
                status = read(clifd, msg_buf, MAX_BUF_LEN);
                printf("\n\033[F\033[2K\r"); // go up one line and clear it
                printf(CBW"(%s)"CCLR": %s\n", username_buf, msg_buf);
                break;
            case P_USER_LIST:
                DEBUG("P_USER_LIST");
                recv_users(clifd);
                break;
            case P_USER_DISCONNECT:
                DEBUG("P_USER_DISCONNECT");
                recv_disconnected(me.clifd);
                break;
            case P_USER_CONNECT:
                DEBUG("P_USER_CONNECT");
                recv_connected(me.clifd);
                break;
            case P_MSG_LIST:
                DEBUG("P_MSG_LIST");
                recv_msg_list(me.clifd);
                break;
            case P_SERVER_END:
                printf("\n");
                INFO("P_SERVER_END");
                exit(0);
                break;
            case P_USER_RENAME:
                INFO("P_USER_RENAME");
                recv_user_rename(me.clifd);
                break;
            default:
                INFO("Unknown msg type: %d", type);
        }
        if (status < 0) break;
        DEBUG("BUF: '%s'", me.buf);
        printf(" "CBC"%s"CCLR" > ", username);
        fflush(stdout);
    }

    close(clifd);

    return 0;
}
