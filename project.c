#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string.h>
#include "project.h"
int get_ip(char *hostname, char **ip) {
    struct hostent *h;

/**
 * The struct hostent (host entry) with its terms documented

    struct hostent {
        char *h_name;    // Official name of the host.
        char **h_aliases;    // A NULL-terminated array of alternate names for the host.
        int h_addrtype;    // The type of address being returned; usually AF_INET.
        int h_length;    // The length of the address in bytes.
        char **h_addr_list;    // A zero-terminated array of network addresses for the host.
        // Host addresses are in Network Byte Order.
    };

    #define h_addr h_addr_list[0]	The first address in h_addr_list.
*/
    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    *ip = (char*) inet_ntoa(*((struct in_addr *) h->h_addr));
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", *ip);

    return 0;
}
int create_socket(char* ip , int port){
    int sockfd;
    struct sockaddr_in server_addr;
    size_t bytes;

    /*server address handling*/
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);    /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(port);        /*server TCP port must be network byte ordered */
    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(-1);
    }
    printf("Ip: %s\n", ip);
    printf("Port: %d\n", port);
    printf("Socket created\n");
    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *) &server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect()");
        exit(-1);
    }
    printf("Connected to %s:%d\n", ip, port);
    return sockfd;
}
int send_message(int sockfd, char *message) {

    printf("Sent %s\n", message);
    /*send a string to the server*/
    int bytes = write(sockfd, message, strlen(message));
    if (bytes > 0)
        printf("Bytes escritos %ld\n", bytes);
    else {
        perror("write()");
        exit(-1);
    }
    return 0;
}
int get_response(int sockfd, char **response) {
    char* buffer = malloc(MAX_SIZE*sizeof(char));
    char byte;
    int i = 0;
    while(1){
        read(sockfd, &byte, 1);
        if(byte == '\n')
            break;
        else if(i == MAX_SIZE){
            printf("Buffer overflow\n");
            break;
        }
        else
            buffer[i++] = byte;
    }
    char temp[4];
    strncpy(temp, buffer, 3); 
    temp[3] = '\0'; // Null terminate the string
    int value = (int)strtol(temp, NULL, 10); 
    *response = buffer;
    return value;
}
int Auth(int sockfd, char* username, char* password){
    char user_message[MAX_SIZE];
    char pass_message[MAX_SIZE];
    char* response = malloc(MAX_SIZE*sizeof(char));
    sprintf(user_message, "USER %s\n", username);
    if(send_message(sockfd, user_message)!= 0){
        return -1;
    }
    if(get_response(sockfd,&response)!= USER_NAME_OK)
        return -1;
    printf("Response: %s\n", response);
    sprintf(pass_message, "PASS %s\n", password);
    if(send_message(sockfd, pass_message)!= 0){
        return -1;
    }
    if(get_response(sockfd,&response)!= LOGIN_OK)
        return -1;
    printf("Response: %s\n", response);
    free(response);
    return 0;
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address to get IP address>\n", argv[0]);
        exit(-1);
    }
    char* ip = malloc(16);
    memset(ip,0,16);
    if(get_ip(argv[1],&ip) < 0) {
        perror("get_ip()");
        exit(-1);
    }
    printf("Ip: %s\n", ip);
    int sockfd = create_socket(ip, 21);
    char* response = malloc(MAX_SIZE*sizeof(char));
    if(get_response(sockfd, &response) != SERVER_READY){
        return -1;
    }
    if(Auth(sockfd, "rcom", "rcom") != LOGIN_OK){
        return -1;
    }
    printf("Response: %s\n", response);
    //free(ip);
    return 0;
}
