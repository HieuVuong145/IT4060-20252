#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

struct Student {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <PORT> <log_file>\n", argv[0]);
        return 1;
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    while (1) {
        int client = accept(server, (struct sockaddr*)&client_addr, &addr_len);

        struct Student sv;
        recv(client, &sv, sizeof(sv), 0);

        // lấy thời gian
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        FILE *f = fopen(argv[2], "a");

        fprintf(f, "%s %04d-%02d-%02d %02d:%02d:%02d %s %s %s %.2f\n",
            inet_ntoa(client_addr.sin_addr),
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            sv.mssv, sv.name, sv.dob, sv.gpa);

        fclose(f);

        printf("Received from %s\n", inet_ntoa(client_addr.sin_addr));

        close(client);
    }

    close(server);
    return 0;
}