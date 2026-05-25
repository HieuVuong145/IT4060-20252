#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

int waiting_client = -1; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Pair { int c1; int c2; };

void *chat_thread(void *arg) {
    struct Pair *p = (struct Pair*)arg;
    int client = p->c1;
    int partner = p->c2;
    free(p); 
    
    char buf[1024], msg[1100];
    while (1) {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            send(partner, "-> Doi tac da thoat!\n", 21, 0);
            close(client);
            close(partner);
            break;
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0; 
        if (strlen(buf) == 0) continue;

        sprintf(msg, "Partner: %s\n", buf);
        send(partner, msg, strlen(msg), 0);
    }
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN); 
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Chat pair server dang chay tren cong 8080...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        pthread_mutex_lock(&mutex);
        if (waiting_client == -1) {
            waiting_client = client;
            send(client, "-> Dang cho 1 nguoi nua...\n", 27, 0);
            pthread_mutex_unlock(&mutex);
        } else {
            int partner = waiting_client;
            waiting_client = -1; 
            pthread_mutex_unlock(&mutex);

            send(partner, "-> Da ghep cap! Bat dau chat.\n", 30, 0);
            send(client, "-> Da ghep cap! Bat dau chat.\n", 30, 0);

            struct Pair *p1 = malloc(sizeof(struct Pair)); p1->c1 = client; p1->c2 = partner;
            pthread_t t1; pthread_create(&t1, NULL, chat_thread, p1); pthread_detach(t1);

            struct Pair *p2 = malloc(sizeof(struct Pair)); p2->c1 = partner; p2->c2 = client;
            pthread_t t2; pthread_create(&t2, NULL, chat_thread, p2); pthread_detach(t2);
        }
    }
    return 0;
}