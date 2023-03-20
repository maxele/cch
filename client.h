#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "options.h"

typedef struct me_t {
    int clifd;
    char *buf;
    char *username;
} me_t;

int client(char username[MAX_USERNAME_LEN]);

#endif // __CLIENT_H__
