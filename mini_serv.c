//
// Created by io on 07.02.23.
//
#include "string.h"
#include "sys/select.h"
#include "sys/socket.h"
#include "unistd.h"
#include "arpa/inet.h"
#include "stdlib.h"
#include "stdio.h"
#include "netinet/in.h"

#define SIZE 65536*2

typedef struct value{
    int max_fd;
    int base_id;
    int* list_id;
    void (*on_free)(struct value *u_value);
} users_value;
typedef struct b_fix{
    fd_set act_set;
    fd_set read_set;
    fd_set wr_set;
    void (*on_equals)(struct b_fix *set);
} buf_fix;
typedef struct mail{
    char* send;
    char* read;
    char* arr;
    void (*on_free)(struct mail *_postman);
}postman;

void on_free_u_value(users_value *u_value);
void putstr_fd(char *str, int fd);
void arg_error();
void fatal_error();
void on_equals(buf_fix *set);
void on_free_postman(postman *_postman);
void send_msg(int fd, char *msg, users_value *u_value ,buf_fix *_fd_set);
void new_register_user(int conn_fd, users_value *u_value, buf_fix *_fd_set, postman *_postman);
void send_msg_read(int fd_out,int len_msg, users_value *u_value , buf_fix *_fd_set, postman *_postman);
void disconnect_client(int dconn_fd, users_value *u_value, buf_fix *_fd_set, postman *_postman);

int main(int argc, char **argv){
    if (argc != 2)
        arg_error();
    int port = atoi(argv[1]),
        serv_fd = socket(AF_INET, SOCK_STREAM, 0),
        conn_fd;
    if (port == -1 || serv_fd == -1)
        fatal_error();
    int client_fd;
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    socklen_t  addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(serv_fd, (struct sockaddr *)&addr, addr_len) == -1)
        fatal_error();
    if(listen(serv_fd, 10) != 0)
        fatal_error();
    users_value u_value = {serv_fd, 0, calloc(1024, sizeof(int)), on_free_u_value};
    buf_fix _fd_set;
    postman _postman = {calloc(SIZE, 1),calloc(SIZE, 1),calloc(SIZE, 1),on_free_postman};
    _fd_set.on_equals = on_equals;
    FD_ZERO(&(_fd_set.act_set));
    FD_SET(serv_fd, &_fd_set.act_set);
    while (1)
    {
        _fd_set.on_equals(&_fd_set);
        if (select((u_value.max_fd) + 1, &(_fd_set.read_set), &(_fd_set.wr_set), NULL, NULL) < 0)
            continue;
        for(int fd = 3;fd <= u_value.max_fd; fd++)
        {
            if(FD_ISSET(fd, &_fd_set.read_set))
            {
                if(fd == serv_fd)
                {
                    conn_fd = accept(serv_fd, (struct sockaddr*)&addr, &addr_len);
                    if(conn_fd < 0)
                        continue;
                    else{
                        new_register_user(conn_fd,&u_value,&_fd_set,&_postman);
                        break;
                    }
                }
                else
                {
                    int len_msg = recv(fd, _postman.read, SIZE, 0);

                    if(len_msg <= 0)
                    {
                        disconnect_client(fd,&u_value,&_fd_set, &_postman);
                        break;
                    }
                    else {
                        send_msg_read(fd, len_msg,&u_value,&_fd_set, &_postman);
                    }
                }
            }
        }
    }
    u_value.on_free(&u_value);
    _postman.on_free(&_postman);
}

void disconnect_client(int dconn_fd, users_value *u_value, buf_fix *_fd_set, postman *_postman){
    sprintf(_postman->send, "server: client %d just left\n", u_value->list_id[dconn_fd]);
    send_msg(dconn_fd, _postman->send,u_value, _fd_set);
    FD_CLR(dconn_fd, &_fd_set->act_set);
    close(dconn_fd);
}

void new_register_user(int conn_fd, users_value *u_value, buf_fix *_fd_set, postman *_postman)
{
    u_value->max_fd = conn_fd > u_value->max_fd ? conn_fd : u_value->max_fd;
    u_value->list_id[conn_fd] = (u_value->base_id)++;
    FD_SET(conn_fd,&_fd_set->act_set);
    sprintf(_postman->send, "server: client %d just arrived\n", u_value->list_id[conn_fd]);
    send_msg(conn_fd, _postman->send, u_value,_fd_set);
}

void send_msg_read(int fd_out,int len_msg,users_value *u_value ,buf_fix *_fd_set, postman *_postman){
    for(int i = 0, j = 0; i < len_msg && i <= SIZE;i++, j++)
    {
        _postman->arr[j] = _postman->read[i];
        if(_postman->arr[j] == '\n')
        {
            _postman->arr[j] = 0;
            sprintf(_postman->send, "client %d: %s\n", u_value->list_id[fd_out], _postman->arr);
            send_msg(fd_out, _postman->send,u_value, _fd_set);
            j = -1;
        }
    }
}

void send_msg(int fd, char *msg, users_value *u_value , buf_fix *_fd_set){
    for(int recfd = 3; u_value->max_fd >= recfd; recfd++)
    {
        if(FD_ISSET(recfd, &_fd_set->wr_set) && recfd != fd)
            send(recfd, msg, strlen(msg), 0);
    }
}

void putstr_fd(char *str, int fd){
    write(fd, str, strlen(str));
}

void arg_error(){
    putstr_fd("Wrong number of arguments\n", 2);
    exit(1);
}

void fatal_error(){
    putstr_fd("Fatal error\n", 2);
    exit(1);
}
void on_equals(buf_fix *set){
    set->read_set = set->wr_set = set->act_set;
}
void on_free_postman(postman *_postman){
    free(_postman->send);
    free(_postman->read);
    free(_postman->arr);
}
void on_free_u_value(users_value *u_value){
    free(u_value->list_id);
}
