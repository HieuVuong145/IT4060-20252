#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENTS 64

int check_login(char *user, char *pass) {
    FILE *f = fopen("user.txt", "r");
    if (f == NULL) {
        perror("Lỗi: Không thể mở file user.txt");
        return 0;
    }
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
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
    addr.sin_port = htons(9090);
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
    int states[MAX_CLIENTS];
    char user[MAX_CLIENTS][50];
    int nclients = 0;
    struct pollfd fds[MAX_CLIENTS + 1];
    printf("telnet server dang chay tren cong 9090...\n");
    while (1) {
        fds[0].fd = listener;
        fds[0].events = POLLIN;
        for (int i = 0; i < nclients; i++) {
            fds[i + 1].fd = clients[i];
            fds[i + 1].events = POLLIN;
        }
        if (poll(fds, nclients + 1, -1) < 0) {
            perror("poll() failed");
            break;
        }
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (client >= 0) {
                printf("client fd = %d da ket noi.\n", client);
                clients[nclients] = client;
                states[nclients] = 0; // Trạng thái 0: Chờ nhập username
                send(client, "username: ", 10, 0);
                nclients++;
            }
        }
        for (int i = 0; i < nclients; i++) {
            if (fds[i + 1].revents & (POLLIN | POLLERR | POLLHUP)) {
                char buf[1024];
                int len = recv(clients[i], buf, sizeof(buf)-1, 0);
                if (len <= 0) {
                    printf("client fd = %d da ngat ket noi.\n", clients[i]);
                    close(clients[i]);                    
                    char trash_file[64];
                    sprintf(trash_file, "out_%d.txt", clients[i]);
                    remove(trash_file);
                    clients[i] = clients[nclients - 1];
                    states[i] = states[nclients - 1];
                    strcpy(user[i], user[nclients - 1]);
                    nclients--;
                    i--;
                    continue;
                }
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;
                if (strlen(buf) == 0) continue;
                if (states[i] == 0) {
                    strcpy(user[i], buf);
                    send(clients[i], "password: ", 10, 0);
                    states[i] = 1;
                }
                else if (states[i] == 1) {
                    if (check_login(user[i], buf)) {
                        char *ok_msg = "login OK, nhap lenh de thuc thi:\n> ";
                        send(clients[i], ok_msg, strlen(ok_msg), 0);
                        states[i] = 2;
                    } else {
                        char *fail_msg = "login failed, vui long thu lai.\nusername: ";
                        send(clients[i], fail_msg, strlen(fail_msg), 0);
                        states[i] = 0;
                    }
                }
                else if (states[i] == 2) {
                    char cmd[1500];
                    char out_file[64];
                    sprintf(out_file, "out_%d.txt", clients[i]);   
                    sprintf(cmd, "%s > %s 2>&1", buf, out_file); 
                    system(cmd);
                    
                    FILE *f = fopen(out_file, "r");
                    if (f) {
                        char fbuf[1024];
                        int bytes_read;
                        while ((bytes_read = fread(fbuf, 1, sizeof(fbuf) - 1, f)) > 0) {
                            fbuf[bytes_read] = 0;
                            send(clients[i], fbuf, bytes_read, 0);
                        }
                        fclose(f);
                    } else {
                        char *err_msg = "Loi: Khong the thuc thi lenh.\n";
                        send(clients[i], err_msg, strlen(err_msg), 0);
                    }                    
                    send(clients[i], "\n> ", 3, 0); 
                }
            }
        }
    }
    close(listener);
    return 0;
}