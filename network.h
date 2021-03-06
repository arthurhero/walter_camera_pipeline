#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "socket.h"

// Open a server socket and return the port number and server socket fd
static void open_camera_connection(unsigned short *port, int *server_socket_fd){
    // Open a server socket
    *server_socket_fd = server_socket_open(port);
    if (*server_socket_fd == -1) {
        perror("Server socket was not opened");
        exit(2);
    }

    // Start listening for connections
    if (listen(*server_socket_fd, 1)) {
        perror("listen failed");
        exit(2);
    }
}

// connect to an existing camera connect 
// return the source socket fd
static void join_camera_connection(char *peer_hostname, unsigned short peer_port, int *socket_fd) {
    *socket_fd = socket_connect(peer_hostname, peer_port);
    if (*socket_fd == -1) {
        perror("Failed to connect");
        exit(2);
    }
}

// accept connection from client 
// return the client port
static void accept_connection(int server_socket_fd, int *client_socket_fd) {
    *client_socket_fd = server_socket_accept(server_socket_fd);
    if (*client_socket_fd == -1) {
        perror("Accept failed");
        exit(2);
    }
    return;
}

// get one int
static int get_int(int fd){
    ssize_t rc;
    int result;
    // Read image row num 
    rc=read(fd, &result, sizeof(int));
    if (rc==-1) {
        return rc;
    }
    return result;
}

//receive camera stream: 3D array - h x w x 3
static void get_one_picture(int fd, unsigned char *image_arr, int height, int width) {
    ssize_t rc;
    size_t total=height*width*3*sizeof(unsigned char);
    size_t cur_chunk=total;
    size_t count = 0;
    while (cur_chunk>0) {
        rc=read(fd, image_arr, cur_chunk);
        if (rc==-1) {
            close(fd);
            exit(2);
        }
        cur_chunk-=(size_t)rc;
        image_arr+=(rc/sizeof(unsigned char));
    }
}

// send an int to client
// if cannot send, we think client quitted, return -1
// On success, return 0
static int send_int(int client_fd, int i) {
    ssize_t rc;
    rc=write(client_fd,&i,sizeof(int));
    if (rc==-1) {
        close(client_fd);
        return -1;
    }
    return 0;
}

// send image to client
// if cannot send, we think client quitted, return -1
// On success, return 0 
static int send_image(int client_fd, int height, int width, unsigned char *image) {
    ssize_t rc;
    // Send image arr to client
    rc=write(client_fd,image,(size_t)(3*width*height*sizeof(unsigned char)));
    if (rc==-1) {
        close(client_fd);
        return -1;
    }
    return 0;
}

#endif
