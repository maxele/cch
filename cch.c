#include "cch.h"
#include "server.h"
#include "client.h"
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void help(char *filename) {
    printf(CBW"CLIENT: "CBC"%s"CCLR" <username> [-p <port>] [-h <hostname>]\n", filename);
    printf(CBW"SERVER: "CBC"%s"CCLR" -s [-f <filename>][-p <port>] [-h <hostname>]\n", filename);
    printf("\n");
    printf("     "CBC"<username>"CCLR"     The username which should be used (max %d)\n", MAX_USERNAME_LEN);
    printf("     "CBC"-s"CCLR"             Enables server mode\n");
    printf("     "CBC"-f <filename>"CCLR"  If given, all messages will be stored up into <filename>\n");
    printf("                    and they will also be restored next time\n");
    printf("     "CBC"-p <port>"CCLR"      The port to which to connect to\n");
    printf("     "CBC"-h <hostname>"CCLR"  The host to which to connect to\n");
    exit(0);
}

// stores the ip of hostname into ip
void hostnametoip(const char *hostname, char ip[16]) {
    struct hostent *lh = gethostbyname(hostname);

    if (!lh) {
        exit(-1);
    } else {
        struct in_addr **addr_list = (struct in_addr **) lh->h_addr_list;     
        strcpy(ip , inet_ntoa(*addr_list[0]));
    }
}

int main(int argc, char *argv[]) {
    bool isserver = false;

    char *username = 0;
    char *filename = 0;
    char *hostname = 0;
    char *port = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            help(argv[0]);
        } else if (strcmp(argv[i], "-p") == 0) {
            if (i+1 < argc && argv[i+1][0] != '-') {
                // DEBUG("Port: %s", argv[i+1]);
                port = argv[i+1];
            } else {
                ERROR("No port given");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-h") == 0) {
            if (i+1 < argc && argv[i+1][0] != '-') {
                // DEBUG("Port: %s", argv[i+1]);
                hostname = argv[i+1];
            } else {
                ERROR("No hostname given");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-f") == 0) {
            if (i+1 < argc && argv[i+1][0] != '-') {
                // DEBUG("Port: %s", argv[i+1]);
                filename = argv[i+1];
            } else {
                ERROR("No filename given");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-s") == 0) {
            // DEBUG("Server");
            isserver = true;
        } else if (argv[i][0] != '-') {
            if (username == 0) {
                // DEBUG("Username: %s", argv[i]);
                username = argv[i];
            } else {
                ERROR("Username already set to %s", username);
                return 0;
            }
        } else {
            ERROR("Unknown option: %s", argv[i]);
            printf("use --help for help\n");
            return 0;
        }
        // printf("%s\n", argv[i]);
    }

    if (username != 0) {
        DEBUG("Username: %s", username);
    } else if (!isserver) {
        ERROR("Username not given but is client");
        INFO("Use --help for help");
        return 0;
    }
    if (hostname == 0) hostname = "localhost";
    if (port == 0) port = "25555";
    int portnum = atoi(port);
    if (portnum < 0 || portnum > 65535) {
        ERROR("Invalid port!");
        return 1;
    }

    DEBUG("Filename: %s", filename);
    DEBUG("Hostname: %s", hostname);
    char ip[16];
    hostnametoip(hostname, ip);
    DEBUG("IP: %s", ip);
    DEBUG("Port: %d", portnum);
    DEBUG("isserver: %d", isserver);

    if (isserver) {
        DEBUG("Calling 'server(%d, %s)'", portnum, filename);
        server(portnum, filename);
    } else {
        DEBUG("Calling 'client(%s, %d, %s)'", username, portnum, ip);
        client(username, portnum, ip);
    }

    return 0;
}
