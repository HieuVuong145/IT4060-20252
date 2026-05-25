#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

void *time_thread(void *arg) {
    int client = *(int*)arg;
    free(arg);
    char buf[256];
    
    char *help = "Nhap dinh dang (VD: dd/mm/yyyy hoac hh:mm:ss): ";
    send(client, help, strlen(help), 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = 0;
        buf[strcspn(buf, "\r\n")] = 0;

        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char time_str[100] = {0};

        if (strcmp(buf, "dd/mm/yyyy") == 0) {
            strftime(time_str, sizeof(time_str), "%d/%m/%Y\n", tm);
        } else if (strcmp(buf, "mm/dd/yyyy") == 0) {
            strftime(time_str, sizeof(time_str), "%m/%d/%Y\n", tm);
        } else if (strcmp(buf, "hh:mm:ss") == 0) {
            strftime(time_str, sizeof(time_str), "%H:%M:%S\n", tm);
        } else {
            strcpy(time_str, "Dinh dang khong hop le!\n");
        }
        
        send(client, time_str, strlen(time_str), 0);
        send(client, help, strlen(help), 0);
    }
    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_port = htons(9001); addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr)); listen(listener, 5);

    printf("Time Server (MT) chay o 9001...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;
        int *pc = malloc(sizeof(int)); *pc = client;
        pthread_t tid; pthread_create(&tid, NULL, time_thread, pc); pthread_detach(tid);
    }
    return 0;
}