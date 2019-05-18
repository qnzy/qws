/*
 * ffws: feature free web server
 * (basically a http/0.9 server)
 */

#include <stdio.h>
#include <stdlib.h>

#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#define BUFLEN 1024
#define PORT 8080 
#define RECV_TIMEOUT 10
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define FATAL(msg) do {fprintf(stderr, "uttpd: %s (%s:%d)\n", msg, __FILE__, __LINE__);exit(EXIT_FAILURE);} while(0);

char * get_line(char *buffer, int *idx, int len) {
    int start = *idx;
    if (start < 0) {
        return NULL;
    }
    int i; 
    for (i = start; i<len; i++) {
        if (buffer[i]=='\n') {
            buffer[i] = '\0';
            break;
        }
    }
    if (i < len-1) {
        *idx = i+1;
    } else {
        *idx = -1;
    }
    return &buffer[start];
    
}
  
void serve(int sockfd) 
{ 
    char buf[BUFLEN]; 
    int len;
    char * ptr;
    struct timeval tv = {.tv_sec = RECV_TIMEOUT}; 
    if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))<0) { 
        FATAL("setsockopt error"); 
    }
    if ((len = recv(sockfd, buf, sizeof(buf), 0))<0) { return; }
    int idx=0;
    while((ptr = get_line(buf, &idx, len))!=NULL) {
        while (isspace(*ptr)) {
            ptr++;
            if (*ptr == '\0') {
                break;
            }
        }
        printf("LINE: %s\n", ptr);
    }

//    ptr = strtok(buf, " ");
//    while(ptr!=NULL) {
//        printf("token: %s\n", ptr);
//        ptr = strtok(NULL, " ");
//    }

    //ptr = tolower(strtok(buf, " ")); if (!strcmp(ptr, "get")) printf("get\n");
    //ptr = strtok(NULL, " "); if (ptr!=NULL) printf("url=%s\n", ptr);
} 

void usage() {
    printf("ffws [DIR [PORT]] [-h]\n");
    printf("serve files on http\n");
    printf("  DIR  : directory to serve, defaults to .\n");
    printf("  PORT : port, defaults to 80\n");
    printf("  -h   : this help\n");
    exit(EXIT_SUCCESS);
}
  
int main(int argc, char ** argv) 
{ 
    int dirset = 0;
    char defaultdir[]=".";
    char * dir = defaultdir;
    unsigned int port = 80;
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-h")) usage();
        else if (!strcmp(argv[i], "--help")) usage();
        else if (!dirset) {dir = argv[i]; dirset = 1;}
        else port = atoi(argv[i]);
        if (port == 0) {usage();}
    }
    printf("starting ffws: serving %s on port %d\n", dir, port);
    for (;;) {
        int sockfd, connfd;
        unsigned int len; 
        struct sockaddr_in addr, cli; 
        int enable = 1;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1) {
            FATAL("socket error"); 
        } 
        if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))<0) { 
            FATAL("setsockopt error"); 
        }
        bzero(&addr, sizeof(addr)); 
        addr.sin_family = AF_INET; 
        addr.sin_addr.s_addr = htonl(INADDR_ANY); 
        addr.sin_port = htons(PORT); 
        if ((bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))) != 0) {
            FATAL("bind error");
        } 
        if ((listen(sockfd, 5)) != 0) {
            FATAL("listen error");
        } 
        len = sizeof(cli); 
        if ((connfd = accept(sockfd, (struct sockaddr*)&cli, &len)) < 0) {
            FATAL("accept error");
        } 
        serve(connfd); 
        close(sockfd); 
    }
} 

