#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef __CCH_H__
#define __CCH_H__

#define PORT 25555
#define HOST "127.0.0.1"

#define MAX_USERNAME_LEN 128
#define MAX_BUF_LEN 1028

#define CLIENT_BUF_INCREMENT 8

#define MSG_LIST_MAX 128 // nr messages (not used atm)
#define MSG_LIST_INCREMENT 512 // bytes
#define MSG_LIST_BACKUP_NR 16 // how many messages inbetween backups

#define P_MSG 0
#define P_USER_LIST 1
#define P_USER_DISCONNECT 2
#define P_USER_CONNECT 3
#define P_MSG_LIST 4
#define P_SERVER_END 5
#define P_USER_RENAME 6

#define CCLR "\033[0;0m"
#define CBC "\033[1;37m"
#define CBW "\033[1;39m"

#define CNR "\033[0;31m"
#define CNG "\033[0;32m"

#define INFO(f_, ...) printf(("\033[3;32m[INFO] "f_"\033[0m\n"), ##__VA_ARGS__)
#define ERROR(f_, ...) printf(("\033[3;31m[ERROR] "f_"\033[0m\n"), ##__VA_ARGS__)

// #define __DEBUG__
#ifdef __DEBUG__
#define DEBUG(f_, ...) printf(("\033[3;34m[DEBUG] "f_"\033[0m\n"), ##__VA_ARGS__)
#else
#define DEBUG(f_, ...)
#endif

typedef struct client_t {
    int clifd;
    pthread_t thread_id;
    char *username;
} client_t;

#endif // __CCH_H__
