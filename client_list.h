#ifndef __CLIENT_LIST_H__
#define __CLIENT_LIST_H__

#include "cch.h"
#include "msg_list.h"

typedef struct client_list_t {
    int max_cap;
    int nr_clients;
    client_t *clients;
} client_list_t;

void client_list_init(client_list_t *client_list);
int client_list_add(client_list_t *client_list, int clifd, char username[MAX_USERNAME_LEN]);
void client_list_del(client_list_t *client_list, int clifd);
void client_list_send(client_list_t *client_list, client_t client);
void client_list_rename(client_list_t *client_list, msg_list_t *msg_list, int clifd);

#endif // __CLIENT_LIST_H__
