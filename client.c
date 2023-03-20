#include "./options.h"

struct me_t {
    int clifd;
    char *buf;
    char *username;
};

void help() {
    printf("HELP:\n");
    printf("  /help or /h      print this message\n");
    printf("  /list or /l      get list of users\n");
    // printf("  /prevmsg or /pr  get old messages\n");
    // printf("  /rn           rename yourself\n");
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
    free(buf);
}

void recv_disconnected(int clifd) {
    int status;
    char username[MAX_USERNAME_LEN];
    status = read(clifd, username, MAX_USERNAME_LEN);
    printf("\033[2K\r"); // go up one line and clear it
    printf("User '%s' disconnected", username);
}

void recv_connected(int clifd) {
    int status;
    char username[MAX_USERNAME_LEN];
    status = read(clifd, username, MAX_USERNAME_LEN);
    printf("\033[2K\r"); // go up one line and clear it
    printf("User '%s' connected", username);
}

void recv_msg_list(int clifd) {
    int status;
    int nr_msg;
    char msg_buf[MAX_BUF_LEN];
    char username_buf[MAX_BUF_LEN];

    status = read(clifd, &nr_msg, sizeof(int));
    if (status < 0) return;
    if (nr_msg < 0) {
        INFO("Server doesn't allow/support previous messages");
        return;
    }

    for (int i = 0; i < nr_msg; i++) {
        status = read(clifd, username_buf, MAX_BUF_LEN);
        if (status < 0) return;
        status = read(clifd, msg_buf, MAX_BUF_LEN);
        printf("\033[2K\r"); // go up one line and clear it
        printf("MSG: %s: %s\n", username_buf, msg_buf);
    }
}

void *send_loop(void *arg) {
    struct me_t me = *(struct me_t *)arg;
    // char buf[MAX_BUF_LEN];

    int pos = 0, status = 0;
    while (status >= 0) {
        memset(me.buf, 0, MAX_BUF_LEN);
        printf("\033[2K\r"); // go up one line and clear it
        printf("%s> ", me.username);
        pos = 0;

        while (pos < MAX_BUF_LEN) {
            me.buf[pos] = getc(stdin);
            me.buf[pos+1] = 0;
            if (pos > 0 && me.buf[pos] == '\n'){
                break;
            } else if (me.buf[pos] == '\n') {
                printf("%s> ", me.username);
            } else {
                pos++;
            }
        }
        me.buf[pos] = 0;

        char id = P_MSG;
        if (strcmp("/help", me.buf) == 0 || strcmp("/h", me.buf) == 0) {
            help();
            continue;
        } else if (strcmp("/list", me.buf) == 0 || strcmp("/l", me.buf) == 0) {
            id = P_USER_LIST;
        // } else if (strcmp("/prevmsg", me.buf) == 0 || strcmp("/pr", me.buf) == 0) {
        //     id = P_MSG_LIST;
        } else if (me.buf[0] == '/') {
            printf("Type /help for help\n");
            continue;
        }

        status = send(me.clifd, &id, sizeof(id), 0);
        if (status < 0) return 0;

        switch (id) {
            case P_MSG:
                printf("\033[F\033[2K\r"); // go up one line and clear it
                printf("ME: %s\n", me.buf);
                DEBUG("(%lu) Sending: %s", strlen(me.buf), me.buf);
                status = send(me.clifd, me.buf, strlen(me.buf), 0);
                break;
            case P_USER_LIST:
                DEBUG("Recieving users");
                break;
            // case P_MSG_LIST:
            //     DEBUG("Recieving old messages");
            //     break;
            default:
                INFO("Invalid id");
                break;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    INFO("Creating client socket");
    int clifd = socket(AF_INET, SOCK_STREAM, 0);
    if (clifd < 0) {
        ERROR("Can't create client socket");
        return 0;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, HOST, &addr.sin_addr) <= 0) {
        ERROR("Can't convert host.");
        return 0;
    }

    INFO("Connecting to server");
    if (connect(clifd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERROR("Can't connect to host!");
        return 0;
    }
    
    char username[MAX_USERNAME_LEN];
    memset(username, 0, MAX_USERNAME_LEN);
    strcpy(username, argv[1]);
    INFO("Sending username (%lu) '%s'", strlen(username), username);
    int status = send(clifd, username, strlen(username)+1, 0);
    if (status < 0) {
        ERROR("Couldn't send username!");
    }

    pthread_t thread_id;

    INFO("Getting previously sent messages");
    {
        char id = P_MSG_LIST;
        status = write(clifd, &id, 1);
    }

    char buf[MAX_BUF_LEN];
    struct me_t me = {clifd, buf, username};
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
                printf("(%s): %s", username_buf, msg_buf);
                break;
            case P_USER_LIST:
                DEBUG("Recieving users (main)");
                recv_users(clifd);
                break;
            case P_USER_DISCONNECT:
                DEBUG("Client disconnected");
                recv_disconnected(me.clifd);
                break;
            case P_USER_CONNECT:
                DEBUG("Client connected");
                recv_connected(me.clifd);
                break;
            // case P_MSG_LIST:
            //     DEBUG("Recieving old messages");
            //     recv_msg_list(me.clifd);
            //     break;
            default:
                INFO("Unknown msg type: %d", type);
        }
        if (status < 0) break;
        printf("\n%s> ", username);
        fflush(stdout);
    }

    close(clifd);

    return 0;
}
