#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Hàm kiểm tra tài khoản
int check_login(char *user, char *pass) {
    FILE *f = fopen("user.txt", "r");
    if (!f) return 0;
    
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
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    signal(SIGCHLD, signal_handler);

    printf("Telnet Server (Multiprocess) dang chay tren cong 9090...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        printf("New client connected!\n");

        if (fork() == 0) {
            close(listener);

            int state = 0;
            char username[50];
            char buf[1024];

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
                } 
                else if (state == 1) {
                    if (check_login(username, buf)) {
                        send(client, "Login OK\n> ", 11, 0);
                        state = 2;
                    } else {
                        send(client, "Login Failed\nUsername: ", 23, 0);
                        state = 0;
                    }
                } 
                else if (state == 2) {
                    char cmd[1500];
                    char out_file[64];
                    // Dùng getpid() làm tên file để các tiến trình không bị ghi đè lên nhau
                    sprintf(out_file, "out_%d.txt", getpid());
                    sprintf(cmd, "%s > %s 2>&1", buf, out_file); 
                    system(cmd);

                    FILE *f = fopen(out_file, "r");
                    if (f) {
                        char fbuf[1024];
                        int bytes_read;
                        while ((bytes_read = fread(fbuf, 1, sizeof(fbuf)-1, f)) > 0) {
                            fbuf[bytes_read] = 0;
                            send(client, fbuf, bytes_read, 0);
                        }
                        fclose(f);
                        remove(out_file);
                    }
                    send(client, "\n> ", 3, 0); 
                }
            }
            // clean
            close(client);
            printf("Client disconnected, child process %d exiting.\n", getpid());
            exit(0);
        }
        close(client);
    }

    close(listener);
    return 0;
}