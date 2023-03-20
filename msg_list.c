#include "msg_list.h"

void msg_list_init(msg_list_t *msg_list) {
    msg_list->enabled = true;
    msg_list->nr = 0;
    msg_list->b_max = 0;
    msg_list->b_used = 0;
    msg_list->buf = malloc(0);
}

// TODO: Fix usernames not being freed
void msg_list_add(msg_list_t *msg_list, char *username, char *msg_buf) {
    if (!msg_list->enabled) {
        DEBUG("msg_list_t not enabled");
        return;
    }

    if (msg_list->nr + 1 >= MSG_LIST_MAX) {
        // remove the first username+message
        DEBUG("Removing first message");
        int off = strlen(msg_list->buf)+1;
        off += strlen(msg_list->buf+off)+1;

        msg_list->nr--;
        msg_list->b_used -= off;
        memcpy(msg_list->buf, msg_list->buf+off, msg_list->b_used);
    }

    int buflen = strlen(username) + strlen(msg_buf) + 2;
    char *buf;
    if (msg_list->b_used + buflen >= msg_list->b_max) {
        // increase size
        buf = msg_list->buf;
        msg_list->b_max += MSG_LIST_INCREMENT;
        msg_list->buf = malloc(msg_list->b_max);
        memcpy(msg_list->buf, buf, msg_list->b_used);
        free(buf);
    }
    buf = malloc(buflen);
    memcpy(buf, username, strlen(username)+1);
    memcpy(buf+strlen(username)+1, msg_buf, strlen(msg_buf)+1);

    int a = strlen(buf)+1;
    // DEBUG("%s: %s", buf, buf+a);
    // DEBUG("Adding '%s' from '%s' to msg_list_t", msg_buf, username);
    memcpy(msg_list->buf+msg_list->b_used, buf, buflen);
    msg_list->b_used += buflen;
    msg_list->nr++;

    free(buf);
}

void msg_list_send(msg_list_t *msg_list, client_t client) {
    if (!msg_list->enabled || msg_list->nr <= 0) {
        INFO("Old messages disabled");
        char id = P_MSG_LIST;
        int status, len = 0;
        status = write(client.clifd, &id, 1);
        status = write(client.clifd, &len, sizeof(len));
    }

    INFO("Sending past messages to user '%s' with fd %d", client.username,
            client.clifd);

    char id = P_MSG_LIST;
    int status;
    status = write(client.clifd, &id, 1);
    status = write(client.clifd, &msg_list->b_used, sizeof(msg_list->b_used));
    status = write(client.clifd, msg_list->buf, msg_list->b_used);
    // int bp = 0, mo = 0;
    // for (int i = 0; i < msg_list->nr; i++) {
    //     mo = strlen(msg_list->buf+bp)+1;
    //     DEBUG("%s: %s", msg_list->buf+bp, msg_list->buf+bp+mo);
    //
    //     bp += mo + strlen(msg_list->buf+bp+mo)+1;
    // }
}


