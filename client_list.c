#include "client_list.h"
#include <sys/socket.h>

void client_list_init(client_list_t *client_list) {
    client_list->max_cap = 8;
    client_list->nr_clients = 0;
    client_list->clients = malloc(sizeof(client_t)*8);
}

// returns the position of the new client in the client_list
int client_list_add(client_list_t *client_list, int clifd, char username[MAX_USERNAME_LEN]) {
    if (client_list->nr_clients >= client_list->max_cap-1) {
        client_t *client_fds_backup = client_list->clients;
        client_list->max_cap += CLIENT_BUF_INCREMENT;
        client_list->clients = malloc(sizeof(client_t)*client_list->max_cap);
        for (int i = 0; i < client_list->nr_clients; i++) {
            client_list->clients[i] = client_fds_backup[i];
        }
        free(client_fds_backup);
    }

    client_t *clientptr = &client_list->clients[client_list->nr_clients];
    clientptr->clifd = clifd;
    clientptr->username = malloc(MAX_USERNAME_LEN);
    strcpy(clientptr->username, username);
    // DEBUG("Copying %s to %s", username, client->username);

    client_list->nr_clients++;

    char buf[MAX_BUF_LEN];
    sprintf(buf, "%s connected", username);
    // msg_list_add("S", buf);

    int status;
    char type = P_USER_CONNECT;
    for (int i = 0; i < client_list->nr_clients; i++) {
        if (client_list->clients[i].clifd == clifd) continue; // dont send to newly connected client
        DEBUG("Sending connect of '%s' to '%s'", username, client_list->clients[i].username);
        status = write(client_list->clients[i].clifd, &type, 1);
        if (status < 0) continue;
        status = write(client_list->clients[i].clifd, username, strlen(username)+1);
    }

    return client_list->nr_clients-1;
}

// TODO: Possible memory leak for username?
void client_list_del(client_list_t *client_list, int clifd) {
    int index = -1;
    for (int i = 0; i < client_list->nr_clients; i++) {
        if (clifd == client_list->clients[i].clifd) {
            index = i;
            break;
        }
    }
    if (index < 0) return;

    DEBUG("Deleting client at pos %d with username '%s'", index, 
            client_list->clients[index].username);

    char username[MAX_USERNAME_LEN];
    sprintf(username, "%s", client_list->clients[index].username);

    close(clifd);
    client_list->nr_clients--;
    memcpy(&client_list->clients[index], 
            &client_list->clients[client_list->nr_clients], sizeof(client_t));

    char buf[MAX_BUF_LEN];
    sprintf(buf, "%s disconnected", username);
    // msg_list_add("S", buf);

    int status;
    char type = 2;
    for (int i = 0; i < client_list->nr_clients; i++) {
        DEBUG("Sending disconnect of '%s' to '%s'", username, client_list->clients[i].username);
        status = write(client_list->clients[i].clifd, &type, 1);
        if (status < 0) break;
        status = write(client_list->clients[i].clifd, username, strlen(username));
    }
}

void client_list_send(client_list_t *client_list, client_t client) {
    int nr_clients = client_list->nr_clients;
    uint32_t buf_len = 0;
    for (int i = 0; i < client_list->nr_clients; i++) {
        buf_len += strlen(client_list->clients[i].username);
    }
    buf_len+=nr_clients-1;
    char buf[buf_len];
    int bpos = 0;
    for (int i = 0; i < client_list->nr_clients; i++) {
        for (int j = 0; j < strlen(client_list->clients[i].username); j++) {
            buf[bpos++] = client_list->clients[i].username[j];
        }
        buf[bpos++] = 0;
    }

    int status;
    char type = 1;
    status = write(client.clifd, &type, 1);

    if (buf_len > 0 && status > 0) {
        status = write(client.clifd, &buf_len, sizeof(uint32_t));
        if (status < 0) return;
        status = write(client.clifd, buf, buf_len);
        DEBUG("UL_SIZE: %d", buf_len);
    } else {
        ERROR("buf_size wrong?");
        return;
    }

    DEBUG("Done sending");
}

void client_list_rename(client_list_t *client_list, int clifd) {
    int index = -1;
    for (int i = 0; i < client_list->nr_clients; i++) {
        if (client_list->clients[i].clifd == clifd) {
            index = i;
            break;
        }
    }
    if (index < 0) return;

    char buf[MAX_USERNAME_LEN*2+2];
    memcpy(buf, client_list->clients[index].username, MAX_USERNAME_LEN);
    int status = read(clifd, client_list->clients[index].username, 
            MAX_USERNAME_LEN);

    DEBUG("Renaming %s to %s", buf, client_list->clients[index].username);
    char id = P_USER_RENAME;
    int buflen = strlen(buf)+strlen(client_list->clients[index].username)+2;
    memcpy(buf+strlen(buf)+1, client_list->clients[index].username, 
            strlen(client_list->clients[index].username)+1);

    for (int i = 0; i < client_list->nr_clients; i++) {
        if (client_list->clients[i].clifd == clifd) break;
        status = write(clifd, &id, 1);
        if (status < 0) continue;
        status = write(clifd, &buf, buflen);
    }
}
