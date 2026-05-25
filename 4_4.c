#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 4

int listener; 
pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_thread(void *arg) {
    int thread_id = *(int*)arg;
    char buf[2048];

    while (1) {
        pthread_mutex_lock(&accept_mutex);
        int client = accept(listener, NULL, NULL);
        pthread_mutex_unlock(&accept_mutex);

        if (client >= 0) {
            int len = recv(client, buf, sizeof(buf)-1, 0);
            if (len > 0) {
                buf[len] = 0;
                char html[512], response[1024];
                sprintf(html, "<html><body><h1>Hello!</h1><p>Worker Thread ID: %d</p></body></html>", thread_id);
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s", strlen(html), html);
                send(client, response, strlen(response), 0);
            }
            close(client); 
        }
    }
    return NULL;
}

int main() {
    listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0}; addr.sin_family = AF_INET; addr.sin_port = htons(8080); addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr)); listen(listener, 10);
    
    printf("HTTP Prethread Server chay tren cong 8080...\n");

    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_t tid;
        pthread_create(&tid, NULL, worker_thread, &thread_ids[i]);
        pthread_detach(tid); 
    }

    while(1) pause();
    close(listener);
    return 0;
}