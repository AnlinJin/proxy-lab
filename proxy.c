#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LINSTENINNGPORT 59824
  

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void transfer_requesthdrs(rio_t *rp,int listenfd, const char *hostname, const char *port, const char *shortUri);
int parse_uri(char *uri, char *filename, char *cgiargs);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_storage client_addr;
    socklen_t client_length;
    char hostname[MAXLINE], portname[MAXLINE];
    if(argc != 2) {
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while(1) {
        client_length = sizeof(client_addr); 
        connfd = Accept(listenfd, (SA *) &client_addr, &client_length); //setup connection
        Getnameinfo((SA *)&client_addr,client_length,hostname,MAXLINE,
        portname, MAXLINE,0);
        printf("Accepted connection from (%s, %s)\n", hostname, portname);
        doit(connfd);  //line:netp:tiny:doit
        Close(connfd); //line:netp:tiny:close
    }
    printf("%s", user_agent_hdr);
    return 0;
}

void doit(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    char port[MAXLINE], hostname[MAXLINE];
    char protocol[MAXLINE],shortUri[MAXLINE];  // e.g. /hub/index.html
    rio_t rio;
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) //line:netp:doit:readrequest
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); //line:netp:doit:parserequest
    
    if (strcasecmp(method, "GET"))
    { //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "proxy does not implement this method");
        return;
    }                       //line:netp:doit:endrequesterr
    //parse uri into hostname, port and short version of uri
    if(strstr(uri,":")) {
        sscanf(uri,"%s//%s:%s/%s", protocol,hostname,port,shortUri);
    }
    else {
        sscanf(uri, "%s//%s/%s", protocol,hostname, shortUri);
        sprintf(port, "80");
    }
    transfer_requesthdrs(&rio,fd, hostname,port,shortUri); //line:netp:doit:readrequesthdrs
    return;
}
/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void transfer_requesthdrs(rio_t *rp, int listenfd,const char *hostname, const char *port, const char *shortUri)
{
    char buf[MAXLINE];
    char key[MAXLINE];
    char value[MAXLINE];
    int  clientfd;
    rio_t rio;   // proxy and server rio
    
    clientfd = open_clientfd(hostname, port);
    rio_readinitb(&rio, clientfd);
    //send header
    sprintf(buf,"GET /%s HTTP/1.0\r\n",shortUri);
    rio_writen(clientfd, buf,strlen(buf));
    sprintf(buf,"Host: %s\r\n", hostname);
    rio_writen(clientfd, buf,strlen(buf));
    sprintf(buf,user_agent_hdr);
    rio_writen(clientfd, buf,strlen(buf));
    sprintf(buf,"Connection: close\r\n");
    rio_writen(clientfd, buf,strlen(buf));
    sprintf(buf,"Proxy-Connection: close\r\n");
    rio_writen(clientfd, buf,strlen(buf));

    rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n"))
    { //line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        if(!strstr(buf, "Host") && !strstr(buf, "User-Agent") && 
        !strstr(buf, "Connection") && !strstr(buf, "Proxy-Connection")) {
            rio_writen(clientfd, buf, strlen(buf));
        }
    }

    //forward the response from the server back to the client
    while(rio_readlineb(&rio,buf, MAXLINE)){
        rio_writen(listenfd,buf, strlen(buf));
    }
    return;
}
/* $end read_requesthdrs */

