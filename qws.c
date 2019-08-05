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

#define RECV_TIMEOUT 10
#define PROGNAME "qws"

#define FATAL(msg) do {fprintf(stderr, PROGNAME": %s (%s:%d)\n", msg, __FILE__, __LINE__);exit(EXIT_FAILURE);} while(0);


struct urlEncodingEl_s {
    uint8_t code;
    char character;
    bool encode;
};

const struct urlEncodingEl_s urlEncodingTable[] = {
    {.code = 0x20, .character = ' ', true},
    {.code = 0x21, .character = '!', true},
    {.code = 0x22, .character = '"', true},
    {.code = 0x23, .character = '#', true},
    {.code = 0x24, .character = '$', true},
    {.code = 0x25, .character = '%', true},
    {.code = 0x26, .character = '&', true},
    {.code = 0x27, .character = '\'', true},
    {.code = 0x28, .character = '(', true},
    {.code = 0x29, .character = ')', true},
    {.code = 0x2a, .character = '*', true},
    {.code = 0x2b, .character = '+', true},
    {.code = 0x2c, .character = ',', true},
    {.code = 0x2d, .character = '-', true},
    {.code = 0x2e, .character = '.', false}, // keep period
    {.code = 0x2f, .character = '/', false}, // keep slash
    {.code = 0x3a, .character = ':', true},
    {.code = 0x2b, .character = ';', true},
    {.code = 0x3c, .character = '<', true},
    {.code = 0x3d, .character = '=', true},
    {.code = 0x3e, .character = '>', true},
    {.code = 0x3f, .character = '?', true},
    {.code = 0x40, .character = '@', true},
    {.code = 0x5b, .character = '[', true},
    {.code = 0x5c, .character = '\\', true},
    {.code = 0x5d, .character = ']', true},
    {.code = 0x7b, .character = '{', true},
    {.code = 0x7c, .character = '|', true},
    {.code = 0x7d, .character = '}', true}
};

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

void decodeStr(char * src, char * dst) {
    int j=0;
    for (int i=0; i<strlen(src); i++) {
        if (src[i]!='%') {
            dst[j++] = src[i];
        } else {
            if (strlen(src)-i<2) {
                break;
            }
            char byte_code[3];
            byte_code[0]=src[++i];
            byte_code[1]=src[++i];
            byte_code[2]='\0';
            uint8_t byte_code_i = strtoul(byte_code, 0, 16);
            int urlEncodingTableLength = sizeof(urlEncodingTable)/sizeof(urlEncodingTable[0]);
            for (int k=0; k<urlEncodingTableLength; k++) {
                if (urlEncodingTable[k].code == byte_code_i) {
                    dst[j++]=urlEncodingTable[k].character;
                    break;
                }
            }
        }
    }
    dst[j++]='\0';
}

void encodeStr(char * src, char * dst) {
    int urlEncodingTableLength = sizeof(urlEncodingTable)/sizeof(urlEncodingTable[0]);
    int k=0;
    char encoding[4];
    bool encoded;
    for (int i=0; i<strlen(src); i++) {
        encoded = false;
        for (int j=0; j<urlEncodingTableLength; j++) {
            if (urlEncodingTable[j].character==src[i] && urlEncodingTable[j].encode) {
                sprintf(encoding,"%%%x", urlEncodingTable[j].code);
                dst[k++]=encoding[0];
                dst[k++]=encoding[1];
                dst[k++]=encoding[2];
                encoded = true;
                break; //only one encoding possible
            }
        }
        if (!encoded) {
            dst[k++]=src[i];
        }
    }
    dst[k++]='\0';

}
  
void serve(int sockfd, char* dir) 
{ 
    const unsigned int rxbuflen = 1024;
    char rxbuf[rxbuflen]; 
    int len;
    char * lineptr;
    struct timeval tv = {.tv_sec = RECV_TIMEOUT}; 
    if ((setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)))<0) { 
        FATAL("setsockopt error"); 
    }
    if ((len = recv(sockfd, rxbuf, sizeof(rxbuf), 0))<0) { return; }
    int idx=0;
    while((lineptr = get_line(rxbuf, &idx, len))!=NULL) {
        char * url = strtok(lineptr, " ");
        char decodedUrl[rxbuflen]; 
        if (url==NULL) { continue; }
        if (!strcmp(url, "GET")) {
            url = strtok(NULL, " ");
            // TODO: collect whole line (currently failing on files with spaces)
            if (url==NULL) { continue; }
            decodeStr(url, decodedUrl);
            printf("url: %s ; decodedUrl: %s\n", url, decodedUrl);
            const unsigned int filepathlen = 1024;
            char filepath[filepathlen];
            if (decodedUrl[0] == '/') {
                snprintf(filepath, filepathlen, "%s.%s", dir, decodedUrl);
            } else {
                snprintf(filepath, filepathlen, "%s%s", dir, decodedUrl);
            }
            printf("GET %s\n", filepath);
            if (isDir(filepath)) {
                if(filepath[strlen(filepath)-1] != '/') {
                    strcat(filepath, "/");
                }
                DIR *d;
                struct dirent *dir;
                d = opendir(filepath);
                sockSend(sockfd, "HTTP/1.0 200 \n\n");
                sockSend(sockfd, "<!DOCTYPE html><html><head><title>DIR</title></head><body>\n");
                while ((dir = readdir(d)) != NULL) {
                    char urlDir[1024]="";
                    char urlDirEncoded[1024]="";
                    sockSend(sockfd, "<li><a href=");
                    if (decodedUrl[0] != '/') { strcat(urlDir, "/"); }
                    strcat(urlDir, decodedUrl);
                    if (decodedUrl[strlen(decodedUrl)-1]!='/') { strcat(urlDir, "/"); }
                    strcat(urlDir, dir->d_name);
                    encodeStr(urlDir, urlDirEncoded);
                    sockSend(sockfd, urlDirEncoded);
                    sockSend(sockfd, ">");
                    sockSend(sockfd, dir->d_name);
                    sockSend(sockfd, "</a>\n");
                }
                closedir(d);
                sockSend(sockfd, "</body></html>\n");
            } else {
                FILE * fp;
                const unsigned int readbuflen = 1024;
                char readbuf[readbuflen];
                if ((fp=fopen(filepath, "rb")) == NULL) {
                    sockSend(sockfd, "HTTP/1.0 404 \n\n");
                    sockSend(sockfd, "<!DOCTYPE html><html>"
                            "<head><title>Error</title></head>"
                            "<body>file not found</body></html>\n");
                    return;
                }
                sockSend(sockfd, "HTTP/1.0 200 \n\n");
                // TODO: use fread, binary not working atm
                while ((fgets(readbuf, readbuflen, fp))!=NULL) {
                    sockSend(sockfd, readbuf);
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
    char dir[1024] = ".";
    unsigned int port = 80;
    for (int i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-h")) usage();
        else if (!strcmp(argv[i], "--help")) usage();
        else if (!dirset) {strcpy(dir, argv[i]); dirset = 1;}
        else port = atoi(argv[i]);
        if (port == 0) {usage();}
    }
    if(dir[strlen(dir)-1] != '/') {
        strcat(dir, "/");
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
        serve(connfd, dir); 
        close(connfd);
        close(sockfd); 
    }
} 

