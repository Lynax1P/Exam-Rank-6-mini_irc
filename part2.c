//
// Created by io on 11.02.23.
//
#include "arpa/inet.h"
#include "sys/socket.h"
#include "sys/select.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "netinet/in.h"
#include "unistd.h"
#define ERROR -1
#define SIZE 64500 * 2
typedef struct client{
    int max_fd;
    int base_fd;
    int *list_fd;
} users_value;
typedef struct mail{
    char *read;
    char *send;
    char *arr;
} postman;
typedef struct fix_buf{
    fd_set read;
    fd_set write;
    fd_set actual;
} _fd_set;

void arg_error(){
    write(2,"Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
    exit(1);
}
void fatal_error(){
    write(2,"Fatal error\n", strlen("Fatal error\n"));
    exit(1);
}
void send_msg(int fd_out, char *msg,users_value *u_value,_fd_set *set){
     for(int fd = 3; fd <= u_value->max_fd; fd++)
        if(fd != fd_out && FD_ISSET(fd, &set->write))
            send(fd, msg, strlen(msg),0);
}
void send_pack_msg(int fd_out, int len_msg,users_value *u_value, postman *_postman, _fd_set *set){
    for (int i = 0, j = 0; i < len_msg; i++,j++) {
        _postman->arr[j] = _postman->read[i];
        if(_postman->arr[j] == '\n')
        {
            _postman->arr[j] = 0;
            sprintf(_postman->send, "client %d: %s\n", u_value->list_fd[fd_out], _postman->arr);
            send_msg(fd_out, _postman->send,u_value, set);
            j = -1;
        }
    }
}
void con_user(int new_fd, users_value *u_value,_fd_set *set,postman *_postman){
    u_value->max_fd = u_value->max_fd < new_fd ? new_fd : u_value->max_fd;
    u_value->list_fd[new_fd] = (u_value->base_fd)++;
    FD_SET(new_fd, &set->actual);
    sprintf(_postman->send, "server : client %d just arrived\n", u_value->list_fd[new_fd]);
    send_msg(new_fd, _postman->send, u_value, set);
}
void discon_user(int close_fd, users_value *u_value,_fd_set *set,postman *_postman){
    sprintf(_postman->send, "server : client %d just left\n", u_value->list_fd[close_fd]);
    send_msg(close_fd,_postman->send, u_value,set);
    FD_CLR(close_fd, &set->actual);
    close(close_fd);
}
int main(int argc,char **argv){
    if(argc != 2)
        arg_error();
    int port = atoi(argv[1]), serv_fd = socket(AF_INET, SOCK_STREAM, 0), connect_user;
    if(port == ERROR || serv_fd == ERROR)
        fatal_error();
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if((bind(serv_fd, (struct sockaddr *)&addr, addr_len) == ERROR) || (listen(serv_fd, 10) == ERROR))
        fatal_error();
    postman _postman = {calloc(SIZE, 1), calloc(SIZE, 1), calloc(SIZE, 1)};
    _fd_set set;
    users_value u_value = {serv_fd, 0, calloc(1024, sizeof(int))};
    FD_ZERO(&set.actual);
    FD_SET(serv_fd, &set.actual);
    while (1)
    {
        set.read = set.write = set.actual;
        if(select(u_value.max_fd + 1, &set.read, &set.write, NULL, NULL) < 0)
            continue;
        for(int fd = 3; fd <= u_value.max_fd; fd++){
            if(FD_ISSET(fd, &set.read)){
                if(fd == serv_fd){
                    if((connect_user = accept(serv_fd, (struct sockaddr *)&addr,&addr_len)) >= 0){
                        con_user(connect_user,&u_value,&set, &_postman);
                        break;
                    }
                }
                else
                {
                    int msg_len = recv(fd, _postman.read, SIZE, 0);
                    if (msg_len < 0){
                        discon_user(fd, &u_value, &set, &_postman);
                        break;
                    }
                    else
                        send_pack_msg(fd,msg_len, &u_value, &_postman, &set);
                }
            }
        }
    }
}