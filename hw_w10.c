#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENTS 64
#define MAX_TOPICS 10
#define MAX_TOPIC_LEN 50

typedef struct {
    int fd;
    int n_topics;
    char topics[MAX_TOPICS][MAX_TOPIC_LEN];
} Client;

// hàm check client đã đăng ký topic chưa
int is_subscribed(Client *c, char *topic) {
    for (int i = 0; i < c->n_topics; i++) {
        if (strcmp(c->topics[i], topic) == 0)
            return 1;
    }
    return 0;
}

// hàm huỷ dky topic
void unsubscribe(Client *c, char *topic) {
    for (int i = 0; i < c->n_topics; i++) {
        if (strcmp(c->topics[i], topic) == 0) {
            if (i != c->n_topics - 1) {
                strcpy(c->topics[i], c->topics[c->n_topics - 1]);
            }
            c->n_topics--;
            return;
        }
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    listen(listener, 5);
    struct pollfd fds[MAX_CLIENTS];
    Client clients[MAX_CLIENTS];

    int nfds = 1;
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    printf("PUB/SUB server dang chay o cong 9000...\n");
    while (1) {
        if (poll(fds, nfds, -1) < 0) {
            perror("poll() failed");
            break;
        }
        // xử lý kết nối mới
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (client >= 0 && nfds < MAX_CLIENTS) {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                
                clients[nfds - 1].fd = client;
                clients[nfds - 1].n_topics = 0;
                
                char *welcome = "lệnh có thể thực hiện: SUB <topic>, UNSUB <topic>, PUB <topic> <msg>\n";
                send(client, welcome, strlen(welcome), 0);
                printf("Client fd=%d da ket noi.\n", client);
                nfds++;
            }
        }
        // xử lý tin nhắn từ client
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                char buf[512];
                int len = recv(fds[i].fd, buf, sizeof(buf)-1, 0);
                // Client ngắt kết nối
                if (len <= 0) {
                    printf("Client fd=%d disconnect\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    clients[i - 1] = clients[nfds - 2];
                    nfds--;
                    i--;
                    continue;
                }
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;
                if (strlen(buf) == 0) continue;
                printf("Nhan lenh tu fd=%d: %s\n", fds[i].fd, buf);
                Client *c = &clients[i - 1];

                // SUB
                if (strncmp(buf, "SUB ", 4) == 0) {
                    char topic[50];
                    sscanf(buf + 4, "%49s", topic);
                    if (!is_subscribed(c, topic) && c->n_topics < MAX_TOPICS) {
                        strcpy(c->topics[c->n_topics++], topic);
                        char msg[100];
                        sprintf(msg, "-> Da DANG KY thanh cong chu de: %s\n", topic);
                        send(fds[i].fd, msg, strlen(msg), 0);
                    } else if (is_subscribed(c, topic)) {
                        send(fds[i].fd, "-> Ban da dang ky chu de nay roi!\n", 35, 0);
                    } else {
                        send(fds[i].fd, "-> Dat gioi han chu de (MAX=10)!\n", 34, 0);
                    }
                }
                // UNSUB
                else if (strncmp(buf, "UNSUB ", 6) == 0) {
                    char topic[50];
                    if (sscanf(buf + 6, "%49s", topic) == 1) {
                        if (is_subscribed(c, topic)) {
                            unsubscribe(c, topic);
                            char msg[100];
                            sprintf(msg, "-> Da HUY DANG KY chu de: %s\n", topic);
                            send(fds[i].fd, msg, strlen(msg), 0);
                        } else {
                            char msg[100];
                            sprintf(msg, "-> Ban chua dang ky chu de: %s\n", topic);
                            send(fds[i].fd, msg, strlen(msg), 0);
                        }
                    } else {
                        send(fds[i].fd, "-> Sai cu phap UNSUB! Dung: UNSUB <topic>\n", 43, 0);
                    }
                }

                // PUB
                else if (strncmp(buf, "PUB ", 4) == 0) {
                    char topic[50], msg[400];
                    if (sscanf(buf + 4, "%49s %399[^\n]", topic, msg) == 2) {
                        char out[512];
                        sprintf(out, "[%s] %s\n", topic, msg);
                        int sent_count = 0;
                        for (int j = 1; j < nfds; j++) {
                            if (j == i) continue;
                            Client *target_client = &clients[j - 1];
                            if (is_subscribed(target_client, topic)) {
                                send(fds[j].fd, out, strlen(out), 0);
                                sent_count++;
                            }
                        }
                        char ack[100];
                        sprintf(ack, "-> Da phat tin nhan toi %d client.\n", sent_count);
                        send(fds[i].fd, ack, strlen(ack), 0);
                    } else {
                        send(fds[i].fd, "-> Sai cu phap PUB! Dung: PUB <topic> <msg>\n", 45, 0);
                    }
                } 
                else {
                    send(fds[i].fd, "-> Lenh khong hop le!\n", 23, 0);
                }
            }
        }
    }
    close(listener);
    return 0;
}