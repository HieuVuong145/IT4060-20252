#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int check_login(char *user, char *pass) {
    FILE *f = fopen("user.txt", "r");
    if (!f) return 0;
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f); return 1;
        }
    }
    fclose(f); return 0;
}

void *client_thread(void *arg) {
    int client = *(int*)arg;
    free(arg);
    int state = 0;
    char username[50], buf[1024];

    send(client, "Username: ", 10, 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0;
        if (strlen(buf) == 0) continue;

        if (state == 0) {
            strcpy(username, buf);
            send(client, "Password: ", 10, 0);
            state = 1;
        } else if (state == 1) {
            if (check_login(username, buf)) {
                send(client, "Login OK\n> ", 11, 0);
                state = 2;
            } else {
                send(client, "Login Failed\nUsername: ", 23, 0);
                state = 0;
            }
        } else if (state == 2) {
            char cmd[1500], out_file[64];
            // Dùng ID của luồng để tránh đụng độ file
            sprintf(out_file, "out_%lu.txt", (unsigned long)pthread_self());
            sprintf(cmd, "%s > %s 2>&1", buf, out_file); 
            system(cmd);

            FILE *f = fopen(out_file, "r");
            if (f) {
                char fbuf[1024]; int bytes;
                while ((bytes = fread(fbuf, 1, sizeof(fbuf)-1, f)) > 0) {
                    send(client, fbuf, bytes, 0);
                }
                fclose(f); remove(out_file);
            }
            send(client, "\n> ", 3, 0); 
        }
    }
    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_port = htons(9090); addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr)); listen(listener, 5);

    printf("Telnet Server (MT) dang chay o 9090...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;
        int *pc = malloc(sizeof(int)); *pc = client;
        pthread_t tid; pthread_create(&tid, NULL, client_thread, pc); pthread_detach(tid);
    }
    return 0;
}