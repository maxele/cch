#include "./options.h"
#include "./client_list.h"
#include <signal.h>

// TODO: Change username to clifd
typedef struct msg_t {
    char *username;
    char *msg;
} msg_t;

struct msg_list_t {
    int max;
    int nr;
    msg_t *msg;
} msg_list;

client_list_t client_list;

// MANUAL:
// -> char[256]:     username (ends in 0)
// <- uint8:         0 if succeded 1 if username taken
// <- uint16:        the number of messages that have been sent
// <- uint8[uint16]: the messages

// TODO: Fix usernames not being freed
void msg_list_add(char *username, char *msg_buf) {
    if (msg_list.nr + 1 >= MSG_LIST_MAX) {
        int inc_bytes = MSG_LIST_INCREMENT*sizeof(msg_t);
        DEBUG("Deleting old messages");

        for (int i = 0; i < msg_list.nr-MSG_LIST_INCREMENT; i++) {
            msg_list.msg[i].username = msg_list.msg[i+MSG_LIST_INCREMENT].username;
        }
        msg_list.nr -= MSG_LIST_INCREMENT;
        DEBUG("Resized down to %d", msg_list.nr);
    } else if (msg_list.nr + 1 >= msg_list.max) {
        int nr_bytes_old = msg_list.nr * sizeof(msg_t);
        msg_list.max += MSG_LIST_INCREMENT;
        int nr_bytes_new = msg_list.nr * sizeof(msg_t);
        DEBUG("Resized msg_list to %d", msg_list.max);

        msg_t *msg_backup = msg_backup = malloc(nr_bytes_old);
        memcpy(msg_backup, msg_list.msg, nr_bytes_old);
        free(msg_list.msg);
        msg_list.msg = malloc(nr_bytes_new);
        memcpy(msg_list.msg, msg_backup, nr_bytes_old);
        free(msg_backup);
    }

    msg_list.msg[msg_list.nr].username = malloc(strlen(username)+1);
    memset(msg_list.msg[msg_list.nr].username, 0, strlen(username)+1);
    memcpy(msg_list.msg[msg_list.nr].username, username, strlen(username));
    msg_list.msg[msg_list.nr].msg = malloc(strlen(msg_buf));
    memset(msg_list.msg[msg_list.nr].msg, 0, strlen(msg_buf)+1);
    memcpy(msg_list.msg[msg_list.nr].msg, msg_buf, strlen(msg_buf));
    msg_list.nr++;
}

void msg_list_send(client_t client) {
    INFO("Sending past messages to user '%s' with fd %d", client.username,
            client.clifd);
    if (msg_list.nr <= 0) return;

    int status;
    int id = P_MSG_LIST;
    status = write(client.clifd, &id, sizeof(id));
    if (status < 0) return;

    status = write(client.clifd, &msg_list.nr, sizeof(msg_list.nr));
    if (status < 0) return;
    for (int i = 0; i < msg_list.nr; i++) {
        // printf("MSG: %s: %s\n", msg_list.msg[i].username, msg_list.msg[i].msg);
        status = write(client.clifd, msg_list.msg[i].username,
                strlen(msg_list.msg[i].username));
        if (status < 0) return;
        status = write(client.clifd, msg_list.msg[i].msg,
                strlen(msg_list.msg[i].msg));
    }
    // DEBUG("Past messages not allowed");
    // id = -1;
    // status = write(client.clifd, &id, sizeof(int));
}

void msg_list_init() {
    msg_list.max = 0;
    msg_list.nr = 0;
    msg_list.msg = malloc(1);
}

void sharemsg(client_t client) {
    char msg_buf[MAX_BUF_LEN];
    memset(msg_buf, 0, MAX_BUF_LEN);

    int status = read(client.clifd, msg_buf, MAX_BUF_LEN);
    if (strlen(msg_buf) <= 0) return;
    // msg_list_add(client.username, msg_buf);

    char type = 0;
    if (status < 0) return;
    // sprintf(total_buf, "(%s): %s", client.username, msg_buf);
    if (status >= 0) {
        printf("(%s): %s\n", client.username, msg_buf);
        for (int i = 0; i < client_list.nr_clients; i++) {
            if (client_list.clients[i].clifd != client.clifd) {
                status = write(client_list.clients[i].clifd, &type, 1);
                DEBUG("Sharing msg from %s to %s", client.username, client_list.clients[i].username);
                status = write(client_list.clients[i].clifd, client.username, MAX_USERNAME_LEN);
                if (status < 0) break;
                status = write(client_list.clients[i].clifd, msg_buf, strlen(msg_buf));
            }
        }
    }
}

void *listenclient(void *arg) {
    client_t client = *(client_t *)arg;

    DEBUG("Creating thread for '%s' (fd: %d)", client.username, client.clifd);

    int status = 1;

    char id;
    while (status > 0) {
        status = read(client.clifd, &id, sizeof(id));
        switch (id) {
            case P_MSG:
                sharemsg(client);
                break;
            case P_USER_LIST:
                client_list_send(&client_list, client);
                break;
            // case P_MSG_LIST:
            //     msg_list_send(client);
            //     break;
            default:
                INFO("Unknown id %d from '%s'", id, client.username);
                break;
        }
    }

    INFO("Client '%s' with fd %d disconnected.", client.username, client.clifd);
    client_list_del(&client_list, client.clifd);

    pthread_exit(0);
}

int handleclient(int clifd) {
    char usernamebuf[MAX_BUF_LEN] = {0};

    INFO("New client connected");
    int status;
    status = read(clifd, usernamebuf, MAX_BUF_LEN);
    if (status < 0) {
        ERROR("Couldn't recieve username!");
        close(clifd);
        return -1;
    }

    DEBUG("checking if username '%s' is valid", usernamebuf);
    if (strlen(usernamebuf) < 3) {
        INFO("username '%s' invalid", usernamebuf);
        close(clifd);
        return -1;
    }

    DEBUG("checking if username '%s' is taken:", usernamebuf);
    bool istaken = false;
    for (int i = 0; i < client_list.nr_clients && !istaken; i++) {
        DEBUG("comparing '%s' : '%s'", usernamebuf, client_list.clients[i].username);
        istaken = strcmp(usernamebuf, client_list.clients[i].username) == 0;
    }

    if (!istaken) {
        int i = client_list_add(&client_list, clifd, usernamebuf);
        pthread_create(&client_list.clients[i].thread_id,
                NULL, listenclient, &client_list.clients[i]);
    } else {
        DEBUG("username '%s' is taken", usernamebuf);
        close(clifd);
        return -1;
    }

    return 0;
}

int init_socket(struct sockaddr_in *addr) {
    int servfd = socket(AF_INET, SOCK_STREAM, 0);

    addr->sin_family = AF_INET;
    addr->sin_port = htons(PORT);
    addr->sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(servfd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
        ERROR("can't set setsockopt");
        return -1;
    }

    if (bind(servfd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        ERROR("Can't create server!");
        return -1;
    }

    if (listen(servfd, 4) < 0) {
        ERROR("Can't listen for some reason.");
        return -1;
    }

    return servfd;
}

int main() {
    sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
    msg_list_init();
    client_list_init(&client_list);

    struct sockaddr_in addr;
    INFO("Initialized server socket");
    int servfd = init_socket(&addr);
     
    int clifd;
    int addrlen = sizeof(addr);

    while (true) {
        clifd = accept(servfd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (clifd < 0) {
            ERROR("Error when listening for client.");
        } else {
            handleclient(clifd);
        }
    }
    // char *msg = "HOILA";
    // send(servfd, msg, strlen(msg), 0);
    
    shutdown(servfd, SHUT_RDWR);

    return 0;
}
