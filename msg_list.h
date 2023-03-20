#ifndef __MSG_LIST_H__
#define __MSG_LIST_H__

#include "options.h"

typedef struct msg_list_t {
    bool enabled;
    int nr;
    int b_max;
    int b_used;
    char *buf;
} msg_list_t;

void msg_list_init(msg_list_t *msg_list);
void msg_list_add(msg_list_t *msg_list, char *username, char *msg_buf);
void msg_list_send(msg_list_t *msg_list, client_t client);

#endif // __MSG_LIST_H__
