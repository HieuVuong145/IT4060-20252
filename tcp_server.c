#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <PORT> <greeting_file> <log_file>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("Server is listening on port %d...\n", port);

    while (1) {
        int client = accept(server, NULL, NULL);

        // đọc file chào
        FILE *f = fopen(argv[2], "r");
        char greet[1024];
        fgets(greet, sizeof(greet), f);
        fclose(f);

        send(client, greet, strlen(greet), 0);

        // ghi log
        FILE *log = fopen(argv[3], "a");

        char buf[1024];
        int len;
        while ((len = recv(client, buf, sizeof(buf)-1, 0)) > 0) {
            buf[len] = 0;
            fprintf(log, "%s", buf);
        }

        fclose(log);
        close(client);
    }

    close(server);
    return 0;
}