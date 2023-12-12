#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include "project.h"
int parser(char* ftp_url , struct Resources* r){
    CURLU *url;
    CURLUcode url_code;

    // Initialize the URL handle
    url = curl_url();
    if (!url) {
        fprintf(stderr, "Error initializing URL handle\n");
        return 1;
    }

    // Parse the URL
    url_code = curl_url_set(url, CURLUPART_URL, ftp_url, 0);
    if (url_code != CURLUE_OK) {
        fprintf(stderr, "Error parsing URL: %s\n", curl_easy_strerror(url_code));
        curl_url_cleanup(url);
        return 1;
    }

    // Extract components
    char *user, *password, *host, *path;
    curl_url_get(url, CURLUPART_USER, &user, 0);
    curl_url_get(url, CURLUPART_PASSWORD, &password, 0);
    curl_url_get(url, CURLUPART_HOST, &host, 0);
    curl_url_get(url, CURLUPART_PATH, &path, 0);

    // Print components
    printf("User: %s\n", user ? user : "N/A");
    printf("Password: %s\n", password ? password : "N/A");
    printf("Host: %s\n", host ? host : "N/A");
    printf("Path: %s\n", path ? path : "N/A");

    // Cleanup
    strcpy(r->user_name , user ? user : "anonymous");
    strcpy(r->password, password ? password : "anonymous");
    strcpy(r->host, host ? host : "ftp.up.pt");
    char* temp = strrchr(path, '/');
    temp++;
    strcpy(r->resource, temp);	
    strcpy(r->path, path);
    curl_free(user);
    curl_free(password);
    curl_free(host);
    curl_free(path);
    curl_url_cleanup(url);
}

int get_ip(char *hostname, char **ip) {
    struct hostent *h;
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
int passiveMode(int sockfd, char** ip, int*port){
    write(sockfd, "PASV\n", 5);
    char* response = malloc(MAX_SIZE*sizeof(char));
    int ip1, ip2, ip3, ip4, port_high, port_low;
    if(get_response(sockfd, &response) != PASSIVE_MODE_OK){
        return -1;

    }
    printf("Response: %s\n", response);
    sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &ip1, &ip2, &ip3, &ip4, &port_high, &port_low);
    sprintf(*ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    *port = port_high * 256 + port_low;
    printf("Ip: %s\n", *ip);
    printf("Port: %d\n", *port);
    free(response);
}
int file_comand(int socket , char* file){
    char* response = malloc(MAX_SIZE*sizeof(char));
    char* message = malloc(MAX_SIZE*sizeof(char));
    sprintf(message, "RETR %s\n", file);
    if(send_message(socket, message)!= 0){
        return -1;
    }
    int result = get_response(socket,&response);
    printf("Response: %s\n", response);
    free(response);
    return result;
}
int file_transfer(int socket, char* file){
    printf("File: %s\n", file);
    FILE* fp = fopen(file, "w");
    char* buffer = malloc(MAX_SIZE*sizeof(char));
    int bytes;
    while(1){
        bytes = read(socket, buffer, MAX_SIZE);
        if(bytes == 0)
            break;
        fwrite(buffer, sizeof(char), bytes, fp);
    }
    fclose(fp);
    free(buffer);
    return 0;
}
int closeApp(int socketA, int socketB){
    char* response = malloc(MAX_SIZE*sizeof(char));
    send_message(socketA, "QUIT\n");
    get_response(socketA,&response) ;
    if(get_response(socketA,&response) !=221){
        printf("Error closing connection\n");
        return -1;
    }
    close(socketA);
    close(socketB);
    return 0;
}
int main(int argc, char *argv[]) {
    struct Resources resource;
    if(argc != 2){
        printf("Usage: %s <ftp url>\n", argv[0]);
        return -1;
    }
    parser(argv[1], &resource);
    char* ip = malloc(16);
    memset(ip,0,16);
    if(get_ip(resource.host,&ip) < 0) {
        perror("get_ip()");
        exit(-1);
    }
    int sockfd = create_socket(ip, 21);
    char* response = malloc(MAX_SIZE*sizeof(char));
    if(get_response(sockfd, &response) != SERVER_READY){
        return -1;
    }
    if(Auth(sockfd, resource.user_name, resource.password) != 0){
        printf("Auth failed\n");
        return -1;
    }
    //free(ip);
    int port;
    passiveMode(sockfd, &ip, &port);
    int sockfd2 = create_socket(ip, port);
    if(file_comand(sockfd, resource.path)!= SV_READY_FOR_DOWNLOAD){
        printf("Error fetching file\n");
        return -1;
    }
    printf("Response: %s\n", resource.resource);
    if(file_transfer(sockfd2, resource.resource) != 0){
        printf("Error fetching file\n");
        return -1;
    }
    if(closeApp(sockfd, sockfd2) != 0){
        printf("Error closing connection\n");
        return -1;
    }

    return 0;
}
