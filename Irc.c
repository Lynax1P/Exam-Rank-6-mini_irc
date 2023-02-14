//
// Created by io on 07.02.23.
//
#include "string.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "stdlib.h"
#include "arpa/inet.h"
#include "sys/select.h"
#include "unistd.h"
#include "stdio.h"

void    putstr_fd(char *str, int fd){
    write(fd, str, strlen(str));
}

void    arg_error(){
    putstr_fd("Wrong number of argument\n", 2);
    exit(1);
}

void    fatal_error(){
    putstr_fd("Fatal error\n",2);
    exit(1);
}

int main(int arc, char** argv){
    if(arc <= 1)
        arg_error();
    int port;
    int serv_fd;
    int client_fd;
    char buf[1024];
    struct  sockaddr_in addr;
    socklen_t addr_len;
    fd_set fds;
    serv_fd = socket(AF_INET, SOCK_STREAM,0);
    addr.sin_family = AF_INET;

    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_len = sizeof(addr);
    if((port = atoi(argv[1]))== -1)
        fatal_error();
    addr.sin_port = htons(port);
    printf("port = %d\n", port);
    if (bind(serv_fd, (struct sockaddr *) &addr, addr_len) == -1)
        fatal_error();
    if (listen(serv_fd, 0) == -1)
        fatal_error();
    while (1){
        if((client_fd = accept(serv_fd,(struct  sockaddr *) &addr, &addr_len)) == -1)
            fatal_error();
        FD_ZERO(&fds);
        FD_SET(client_fd, &fds);
        select(client_fd + 1, &fds, NULL, NULL, NULL);
        if(FD_ISSET(client_fd, &fds)){
            recv(client_fd, buf, 1024, 0);
            printf("Recv: %s\n", buf);
        }
    }
    return 0;
}