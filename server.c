#include "server.h"
#include "msg_list.h"

client_list_t client_list;
msg_list_t msg_list;
char *msg_list_file;

// MANUAL:
// -> char[256]:     username (ends in 0)
// <- uint8:         0 if succeded 1 if username taken
// <- uint16:        the number of messages that have been sent
// <- uint8[uint16]: the messages

void sigInt(int s) {
    INFO("Closing server");
    char type = P_SERVER_END;
    for (int i = 0; i < client_list.nr_clients; i++) {
        write(client_list.clients[i].clifd, &type, 1);
        DEBUG("Sharing P_SERVER_END to %s", client_list.clients[i].username);
    }

    if (msg_list_file != 0) {
        INFO("Writing msg_list to file");
        msg_list_write(&msg_list, msg_list_file);
    }
    exit(0);
}

void sendAll(client_t client, char type, char *buf) {
    DEBUG("Sending '%s' with type (%d) to all from %s", buf, type, client.username);

    int status;
    for (int i = 0; i < client_list.nr_clients; i++) {
        if (client_list.clients[i].clifd != client.clifd) {
            status = write(client_list.clients[i].clifd, &type, 1);
            if (status < 0) break;
            DEBUG("Sharing msg from %s to %s", client.username, client_list.clients[i].username);
            status = write(client_list.clients[i].clifd, buf, strlen(buf));
        }
    }
}

void sharemsg(client_t client) {
    char msg_buf[MAX_BUF_LEN];
    memset(msg_buf, 0, MAX_BUF_LEN);

    int status = read(client.clifd, msg_buf, MAX_BUF_LEN);
    if (strlen(msg_buf) <= 0) return;
    msg_list_add(&msg_list, client.username, msg_buf);

    char type = 0;
    if (status < 0) return;
    // sprintf(total_buf, "(%s): %s", client.username, msg_buf);
    if (status < 0) return;

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
        case P_MSG_LIST:
            msg_list_send(&msg_list, client);
            break;
        case P_USER_RENAME:
            client_list_rename(&client_list, client.clifd);
            break;
        default:
            INFO("Unknown id %d from '%s'", id, client.username);
            break;
        }
    }

    INFO("Client '%s' with fd %d disconnected.", client.username, client.clifd);
    client_list_del(&client_list, client.clifd);
    msg_list_add(&msg_list, "D", client.username);

    pthread_exit(0);
}

int handleclient(int clifd) {
    char usernamebuf[MAX_USERNAME_LEN] = {0};

    INFO("New client connected");
    int status;
    status = read(clifd, usernamebuf, MAX_USERNAME_LEN);
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
        msg_list_add(&msg_list, "C", client_list.clients[i].username);
    } else {
        DEBUG("username '%s' is taken", usernamebuf);
        close(clifd);
        return -1;
    }

    return 0;
}

int init_socket(struct sockaddr_in *addr, int port) {
    int servfd = socket(AF_INET, SOCK_STREAM, 0);

    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
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

int server(int port, char *filename) {
    msg_list_file = filename;
    sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
    signal(SIGINT, sigInt);

    msg_list_init(&msg_list);
    client_list_init(&client_list);

    if (msg_list_file != 0) {
        msg_list_read(&msg_list, msg_list_file);
    }

    struct sockaddr_in addr;
    INFO("Initialized server socket");
    int servfd = init_socket(&addr, port);
     
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
