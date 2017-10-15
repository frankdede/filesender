#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "3490"
#define MAX_DATA_SIZE 65535

void * get_in_addr(struct sockaddr * sa) {
    if (sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;

    char buf[MAX_DATA_SIZE];
    struct addrinfo hints, *res, *res0;
    int rv;
    char str_address[INET6_ADDRSTRLEN];

    if (argc !=4) {
        fprintf(stderr, "usage: [source file path] [remote host] [target file path]\n");
        exit(1);
    }

    char *source_file_path = argv[1];
    char *target_file_path = argv[3];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv = getaddrinfo(argv[2], PORT, &hints, &res0)) < 0){
        fprintf(stderr, "getaddrinfo: %s \n", gai_strerror(rv));
        return 1;
    }

    for(res = res0; res != NULL; res = res->ai_next){
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if (sockfd < 0){
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (res == NULL){
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(res->ai_family, get_in_addr(res -> ai_addr), str_address, sizeof(str_address));

    printf("Client: connecting to %s\n", str_address);

    freeaddrinfo(res0);

    FILE *file = fopen(source_file_path, "r");

    if(!file){
        fprintf(stderr, "Fail to read file: %s", source_file_path);
        close(sockfd);
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    long source_file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    size_t target_file_path_size = strlen(target_file_path);

    if (send(sockfd, &source_file_size, sizeof(long), 0) < 0) {
        perror("send source file size");
        exit(1);
    }

    if (send(sockfd, &target_file_path_size, sizeof(size_t), 0) < 0) {
        perror("send target file path size");
        exit(1);
    }

    if (send(sockfd, target_file_path, target_file_path_size, 0) < 0) {
        perror("send target file path");
        exit(1);
    }

    long bytes_read = 0;
    while((bytes_read = fread(buf, sizeof(char), MAX_DATA_SIZE, file)) > 0) {
        if (send(sockfd, buf, bytes_read, 0) < 0) {
            perror("send file");
            exit(1);
        }
    }

    close(sockfd);

    return 0;
}