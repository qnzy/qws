/*
 * qws: web server
 * yves kunz 2019
 * Public Domain / CC0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <dirent.h>

#define BUFLEN 1024
#define RECV_TIMEOUT 10
#define PROGNAME "qws"

#define FATAL(msg) do {fprintf(stderr, PROGNAME": %s (%s:%d)\n", msg, __FILE__, __LINE__);exit(EXIT_FAILURE);} while(0);

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

bool isDir(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void sockSend(int sockfd, const char* str) {
    send(sockfd, str, strlen(str), 0);
}
  
void serve(int sockfd) 
{ 
    char buf[BUFLEN]; 
    int len;
    char * lineptr;
    struct timeval tv = {.tv_sec = RECV_TIMEOUT}; 
    if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))<0) { 
        FATAL("setsockopt error"); 
    }
    if ((len = recv(sockfd, buf, sizeof(buf), 0))<0) { return; }
    int idx=0;
    while((lineptr = get_line(buf, &idx, len))!=NULL) {
        char * tokptr = strtok(lineptr, " ");
        if (tokptr==NULL) { continue; }
        if (!strcmp(tokptr, "GET")) {
            //printf("GET: ");
            tokptr = strtok(NULL, " ");
            if (tokptr==NULL) { continue; }
            char filepath[1024];
            if (tokptr[0] == '/') {
                filepath[0] = '.';
                filepath[1] = '\0';
                strcat(filepath, tokptr);
            } else {
                filepath[0] = '\0';
                strcat(filepath, tokptr);
            }
            if (isDir(filepath)) {
                DIR *d;
                struct dirent *dir;
                d = opendir(filepath);
                sockSend(sockfd, "HTTP/1.0 200 \n\n");
                sockSend(sockfd, "<!DOCTYPE html><html><head><title>DIR</title></head><body>\n");
                while ((dir = readdir(d)) != NULL) {
                    sockSend(sockfd, "<li>\n");
                    sockSend(sockfd, dir->d_name);
                }
                closedir(d);
                sockSend(sockfd, "</body></html>\n");
                printf(" DIR ");
            } else {
                FILE * fp;
                const int bufsize = 1024;
                char buf[bufsize];
                if ((fp=fopen(filepath, "r")) == NULL) {
                    sockSend(sockfd, "HTTP/1.0 404 \n\n");
                    sockSend(sockfd, "<!DOCTYPE html><html>"
                            "<head><title>Error</title></head>"
                            "<body>file not found</body></html>\n");
                    return;
                }
                sockSend(sockfd, "HTTP/1.0 200 \n\n");
                while ((fgets(buf, bufsize, fp))!=NULL) {
                    sockSend(sockfd, buf);
                }
            }
        }
    }
} 

void usage() {
    printf(PROGNAME" [DIR [PORT]] [-h]\n");
    printf("serve files on http\n");
    printf("  DIR  : directory to serve, defaults to current directory\n");
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
    printf("starting "PROGNAME": serving %s on port %d\n", dir, port);
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
        addr.sin_port = htons(port); 
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
        close(connfd);
        close(sockfd); 
    }
} 

