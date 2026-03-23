#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connect failed\n");
        return 1;
    }

    char buf[1024];
    while (1) {
        printf("Enter message: ");
        fgets(buf, sizeof(buf), stdin);

        send(client, buf, strlen(buf), 0);
    }

    close(client);
    return 0;
}