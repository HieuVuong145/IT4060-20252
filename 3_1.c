#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);
    signal(SIGCHLD, sigchld_handler);

    printf("File Server (Multiprocess) chay o cong 9000...\n");

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        if (fork() == 0) {
            close(listener);
            DIR *d = opendir(".");
            struct dirent *dir;
            int file_count = 0;
            char file_list[4096] = "";

            while ((dir = readdir(d)) != NULL) {
                if (dir->d_type == DT_REG) {
                    file_count++;
                    strcat(file_list, dir->d_name);
                    strcat(file_list, "\r\n");
                }
            }
            closedir(d);

            if (file_count == 0) {
                char *err = "ERROR No files to download\r\n";
                send(client, err, strlen(err), 0);
                close(client);
                exit(0);
            }

            char header[256];
            sprintf(header, "OK %d\r\n%s\r\n", file_count, file_list);
            send(client, header, strlen(header), 0);

            char buf[256];
            while (1) {
                int len = recv(client, buf, sizeof(buf)-1, 0);
                if (len <= 0) break;
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0; 

                FILE *f = fopen(buf, "rb");
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long fsize = ftell(f);
                    fseek(f, 0, SEEK_SET);

                    char ok_msg[64];
                    sprintf(ok_msg, "OK %ld\r\n", fsize);
                    send(client, ok_msg, strlen(ok_msg), 0);

                    char fbuf[1024];
                    int bytes_read;
                    while ((bytes_read = fread(fbuf, 1, sizeof(fbuf), f)) > 0) {
                        send(client, fbuf, bytes_read, 0);
                    }
                    fclose(f);
                    close(client);
                    break;
                } else {
                    char *err = "ERROR File not found. Try again:\r\n";
                    send(client, err, strlen(err), 0);
                }
            }
            close(client);
            exit(0);
        }
        close(client);
    }
    return 0;
}