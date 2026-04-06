#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Cú pháp: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_s);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind error");
        close(sock);
        return 1;
    }

    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port_d);
    dest.sin_addr.s_addr = inet_addr(ip_d);

    unsigned long ul = 1;
    ioctl(sock, FIONBIO, &ul);
    ioctl(STDIN_FILENO, FIONBIO, &ul);

    char buf[1024];
    socklen_t len = sizeof(dest);

    printf("chat on gate %d...\n", port_s);
    printf("connect to %s:%d\n", ip_d, port_d);
    printf("you: ");
    fflush(stdout);

    while (1) {
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&dest, &len);
        if (n > 0) {
            buf[n] = 0;
            printf("\rfriend: %s", buf);
            printf("You: ");
            fflush(stdout);
        }

        int m = read(STDIN_FILENO, buf, sizeof(buf) - 1);
        if (m > 0) {
            buf[m] = 0;
            sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)&dest, sizeof(dest));
            printf("you: ");
            fflush(stdout);
        }

        usleep(10000); 
    }

    close(sock);
    return 0;
}