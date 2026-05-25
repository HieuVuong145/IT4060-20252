#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLI 64

int clients[MAX_CLI];
char ids[MAX_CLI][50];
int registered[MAX_CLI] = {0};
int nclients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void remove_client(int client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < nclients; i++) {
        if (clients[i] == client) {
            clients[i] = clients[nclients - 1];
            registered[i] = registered[nclients - 1];
            strcpy(ids[i], ids[nclients - 1]);
            nclients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    close(client);
}

void *client_thread(void *arg) {
    int client = *(int*)arg;
    free(arg);
    char buf[256];
    int my_idx = -1;

    send(client, "Nhap ID (client_id: name): ", 27, 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;

        pthread_mutex_lock(&clients_mutex);
        // Tìm index của chính mình
        for (int i = 0; i < nclients; i++) {
            if (clients[i] == client) { my_idx = i; break; }
        }

        if (!registered[my_idx]) {
            char temp_id[50], temp_name[50];
            if (sscanf(buf, "%49[^:]: %49s", temp_id, temp_name) == 2) {
                strcpy(ids[my_idx], temp_id);
                registered[my_idx] = 1;
                send(client, "Dang ky thanh cong!\n", 20, 0);
            } else {
                send(client, "Sai format!\n", 12, 0);
            }
        } else {
            char msg[512];
            sprintf(msg, "%s: %s\n", ids[my_idx], buf);
            for (int i = 0; i < nclients; i++) {
                if (clients[i] != client && registered[i]) {
                    send(clients[i], msg, strlen(msg), 0);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    remove_client(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_port = htons(8080); addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr)); listen(listener, 5);

    printf("Chat Server (MT) chay o 8080...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        pthread_mutex_lock(&clients_mutex);
        clients[nclients] = client;
        registered[nclients] = 0;
        nclients++;
        pthread_mutex_unlock(&clients_mutex);

        int *pclient = malloc(sizeof(int));
        *pclient = client;
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, pclient);
        pthread_detach(tid);
    }
    return 0;
}