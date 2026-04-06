#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_CLIENTS 64

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Chuyen socket listener sang non-blocking
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    // Server is now listening for incoming connections
    printf("Server is listening on port 8080...\n");

    int clients[MAX_CLIENTS];
    int states[MAX_CLIENTS];
    char hoten[MAX_CLIENTS][100];
    char mssv[MAX_CLIENTS][20];
    int nclients = 0;

    char buf[256];
    int len;

    while (1) {
        // chap nhan ket noi
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New client accepted: %d\n", client);

            clients[nclients] = client;
            states[nclients] = 0;

            ioctl(client, FIONBIO, &ul);

            send(client, "Nhap ho va ten: ", 17, 0);

            nclients++;
        }

        // nhan du lieu tu cac client
        for (int i = 0; i < nclients; i++) {
            len = recv(clients[i], buf, sizeof(buf) - 1, 0);

            if (len <= 0) continue;

            buf[len] = 0;

            printf("Received from %d: %s", clients[i], buf);

            if (states[i] == 0) {
                strcpy(hoten[i], buf);
                send(clients[i], "Nhap MSSV: ", 12, 0);
                states[i] = 1;
            }
            else if (states[i] == 1) {
                strcpy(mssv[i], buf);

                // xu ly ho ten
                char temp[100];
                strcpy(temp, hoten[i]);

                char *words[10];
                int count = 0;

                char *token = strtok(temp, " \n");
                while (token != NULL) {
                    words[count++] = token;
                    token = strtok(NULL, " \n");
                }

                char ten[50];
                strcpy(ten, words[count - 1]);

                for (int j = 0; j < strlen(ten); j++)
                    if (ten[j] >= 'A' && ten[j] <= 'Z')
                        ten[j] += 32;

                char initial[10] = "";
                for (int j = 0; j < count - 1; j++) {
                    char c = words[j][0];
                    if (c >= 'A' && c <= 'Z') c += 32;
                    int l = strlen(initial);
                    initial[l] = c;
                    initial[l + 1] = 0;
                }

                char *ms = mssv[i];
                if (strlen(ms) > 2) ms += 2;

                char email[100];
                sprintf(email, "%s.%s%s@sis.hust.edu.vn\n",
                        ten, initial, ms);

                send(clients[i], email, strlen(email), 0);
                states[i] = 0;
                send(clients[i], "Nhap ho va ten: ", 17, 0);
            }
        }
    }

    close(listener);
    return 0;
}