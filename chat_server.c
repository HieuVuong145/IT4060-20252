#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>

#define MAX_CLIENTS 64

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    int clients[MAX_CLIENTS];
    char ids[MAX_CLIENTS][50];
    int registered[MAX_CLIENTS];
    int nclients = 0;
    fd_set readfds;
    printf("chat server dang chay o cong 8080...\n");
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        int maxfd = listener;

        for (int i = 0; i < nclients; i++) {
            FD_SET(clients[i], &readfds);
            if (clients[i] > maxfd) 
                maxfd = clients[i];
        }

        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select() failed");
            break;
        }

        if (FD_ISSET(listener, &readfds)) {
            int client = accept(listener, NULL, NULL);
            if (client >= 0) {
                printf("client moi ket noi (fd = %d)\n", client);
                clients[nclients] = client;
                registered[nclients] = 0;

                char *msg_ask = "vui long nhap ten (cu phap: client_id: client_name): ";
                send(client, msg_ask, strlen(msg_ask), 0);

                nclients++;
            }
        }
        for (int i = 0; i < nclients; i++) {
            if (FD_ISSET(clients[i], &readfds)) {
                char buf[256];
                int len = recv(clients[i], buf, sizeof(buf)-1, 0);
                if (len <= 0) {
                    printf("Client fd = %d da ngat ket noi.\n", clients[i]);
                    close(clients[i]);
                    clients[i] = clients[nclients - 1];
                    registered[i] = registered[nclients - 1];
                    strcpy(ids[i], ids[nclients - 1]);
                    nclients--;
                    i--;
                    continue;
                }
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;
                if (strlen(buf) == 0) continue;
                if (!registered[i]) {
                    char temp_id[50], temp_name[50];
                    if (sscanf(buf, "%49[^:]: %49s", temp_id, temp_name) == 2) {
                        strcpy(ids[i], temp_id);
                        registered[i] = 1;
                        char *msg_success = "dang ky thanh cong\n";
                        send(clients[i], msg_success, strlen(msg_success), 0);
                        printf("client fd = %d dang ky ID la '%s'\n", clients[i], ids[i]);
                    } else {
                        char *msg_err = "sai cu phap, nhap lai (VD: abc: nguyenvanA): ";
                        send(clients[i], msg_err, strlen(msg_err), 0);
                    }
                } 
                else {
                    char msg[512];
                    time_t t = time(NULL);
                    struct tm *tm_info = localtime(&t);
                    char time_str[50];
                    strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", tm_info);
                    snprintf(msg, sizeof(msg), "%s %s: %s\n", time_str, ids[i], buf);
                    printf("broadcast: %s", msg);
                    for (int j = 0; j < nclients; j++) {
                        if (j != i && registered[j]) {
                            send(clients[j], msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}