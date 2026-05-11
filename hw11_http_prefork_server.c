#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_WORKERS 4

void handle_client(int client) {
    char buf[2048];
    int len = recv(client, buf, sizeof(buf) - 1, 0);
    
    if (len > 0) {
        buf[len] = 0;
        printf("[Worker PID %d] Nhận được yêu cầu kết nối HTTP.\n", getpid());

        char html_content[512];
        sprintf(html_content, "<html><body><h1>Hello from Prefork Server!</h1><p>Phuc vu boi Worker PID: <b>%d</b></p></body></html>", getpid());

        char response[1024];
        sprintf(response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %lu\r\n"
                "Connection: close\r\n\r\n"
                "%s", 
                strlen(html_content), html_content);

        send(client, response, strlen(response), 0);
    }
    
    close(client);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    printf("HTTP Prefork Server dang chay o cong 8080...\n");

    for (int i = 0; i < NUM_WORKERS; i++) {
        if (fork() == 0) {
            printf("Worker %d (PID: %d) da san sang!\n", i + 1, getpid());
            while (1) {
                int client = accept(listener, NULL, NULL);
                if (client >= 0) {
                    handle_client(client);
                }
            }
            exit(0);
        }
    }
    while (wait(NULL) > 0);
    close(listener);
    return 0;
}