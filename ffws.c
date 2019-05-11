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

#define FATAL(msg) do {fprintf(stderr, "uttpd: %s (%s:%d)\n", msg, __FILE__, __LINE__);exit(EXIT_FAILURE);} while(0);
  
void serve(int sockfd) 
{ 
    char buff[BUFLEN]; 
    int len;
    struct timeval tv = {.tv_sec = RECV_TIMEOUT}; 
    if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))<0) { FATAL("setsockopt error"); }
    if ((len = recv(sockfd, buff, sizeof(buff), 0))<0) { return; }
    printf("%s\n", buff);
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
    // TODO: 
    // - parse options (path, port)
    // - handle requests
    // - some kind of timeout

    int dirset = 0, portset = 0;
    char * dir, * portstr;
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-h")) usage();
        if (!strcmp(argv[i], "--help")) usage();
        else if (!dirset) {dir = argv[i]; dirset = 1;}
        else {portstr = argv[i]; portset = 1;}
    }
    if (dirset) printf("dir=%s\n", dir); else printf("dir=.\n");
    if (portset) printf("port=%s\n", portstr); else printf("port=80\n");
    for (;;) {
        int sockfd, connfd;
        unsigned int len; 
        struct sockaddr_in addr, cli; 
        int enable = 1;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))==-1) { FATAL("socket error"); } 
        if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))<0) { FATAL("setsockopt error"); }
        bzero(&addr, sizeof(addr)); 
        addr.sin_family = AF_INET; 
        addr.sin_addr.s_addr = htonl(INADDR_ANY); 
        addr.sin_port = htons(PORT); 
        if ((bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))) != 0) { FATAL("bind error"); } 
        if ((listen(sockfd, 5)) != 0) { FATAL("listen error"); } 
        len = sizeof(cli); 
        if ((connfd = accept(sockfd, (struct sockaddr*)&cli, &len)) < 0) { FATAL("accept error"); } 
        serve(connfd); 
        close(sockfd); 
    }
} 

