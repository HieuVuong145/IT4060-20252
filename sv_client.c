#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

struct Student {
    char mssv[20];
    char name[50];
    char dob[20];
    float gpa;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    int client = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    connect(client, (struct sockaddr*)&server, sizeof(server));

    struct Student sv;

    printf("MSSV: "); fgets(sv.mssv, 20, stdin);
    printf("Name: "); fgets(sv.name, 50, stdin);
    printf("DOB: "); fgets(sv.dob, 20, stdin);
    printf("GPA: "); scanf("%f", &sv.gpa);

    send(client, &sv, sizeof(sv), 0);

    close(client);
    return 0;
}