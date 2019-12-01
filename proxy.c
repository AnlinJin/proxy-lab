#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LINSTENINNGPORT 59824
#define SBUFSIZE 16
#define NTHREADS 4

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void transfer_requesthdrs(rio_t *rp, int listenfd, char *hostname, char *port, char *shortUri);
void parseUrl(char *url, char *hostname, char *port);
//int parse_uri(char *uri, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
void* thread(void * vargp);
void sigpipe_handler(int sig);

sbuf_t sbuf;
cache_t *cache;
int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_storage client_addr;
    socklen_t client_length;
    char hostname[MAXLINE], portname[MAXLINE];
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf, SBUFSIZE);
    cache = init_cache();

    signal(SIGPIPE, sigpipe_handler);

    for(int i = 0; i < NTHREADS; i++) {
        pthread_create(&tid, NULL, thread, NULL);
    }
    while (1)
    {
        client_length = sizeof(client_addr);
        connfd = Accept(listenfd, (SA *)&client_addr, &client_length); //setup connection
        sbuf_insert(&sbuf,connfd);
        Getnameinfo((SA *)&client_addr, client_length, hostname, MAXLINE,
                    portname, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, portname);
        //doit(connfd);  //line:netp:tiny:doit
        //Close(connfd); //line:netp:tiny:close
    }
    
    return 0;
}

void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; 
    char port[MAXLINE], hostname[MAXLINE];
    
    rio_t rio;
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) 
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); 

    if (strcasecmp(method, "GET"))
    { 
        clienterror(fd, method, "501", "Not Implemented",
                    "proxy does not implement this method");
        return;
    } 
    parseUrl(uri, hostname, port);
    //parse uri into hostname, port and short version of uri
    /*if(strstr(uri,":")) {
        sscanf(uri,"http://:%s/%s", protocol,hostname,port,shortUri);
    }
    else {
        sscanf(uri, "%s://%s/%s", protocol,hostname, shortUri);
        sprintf(port, "80");
    }*/
    transfer_requesthdrs(&rio, fd, hostname, port, uri); 
    return;
}
/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void transfer_requesthdrs(rio_t *rp, int listenfd, char *hostname, char *port, char *shortUri)
{
    char * buf = malloc(sizeof(char) * MAX_OBJECT_SIZE);
    //char key[MAXLINE];
    //char value[MAXLINE];
    int clientfd;
    rio_t rio; // proxy and server rio
    char flag = 0;
    int numberOfBytes = 0;
    int cumulative_bytes = 0;
    char * data = read_cache(cache,hostname, shortUri);
    if(data != NULL) {
         rio_writen(listenfd, data, MAX_OBJECT_SIZE);
         printf("return object from cache\n");
         return;
    }

    clientfd = open_clientfd(hostname, port);
    rio_readinitb(&rio, clientfd);
    //send header
    sprintf(buf, "GET %s HTTP/1.0\r\n", shortUri);
    rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "%s", user_agent_hdr);
    rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Connection: close\r\n");
    rio_writen(clientfd, buf, strlen(buf));
    sprintf(buf, "Proxy-Connection: close\r\n");
    rio_writen(clientfd, buf, strlen(buf));

    rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n"))
    { //line:netp:readhdrs:checkterm
        Rio_readlineb(rp, buf, MAXLINE);
        if (strstr(buf, "Host"))
        {
            rio_writen(clientfd, buf, strlen(buf));
            flag = 1;
        }
        if (!strstr(buf, "Host") && !strstr(buf, "User-Agent") &&
            !strstr(buf, "Connection") && !strstr(buf, "Proxy-Connection"))
        {
            rio_writen(clientfd, buf, strlen(buf));
        }
    }
    if (flag == 0)
    {
        sprintf(buf, "Host: %s\r\n", hostname);
        rio_writen(clientfd, buf, strlen(buf));
    }

    //forward the response from the server back to the client

    /*while (rio_readlineb(&rio, buf, MAXLINE)&& strcmp(buf, "\r\n"))
    {
        rio_writen(listenfd, buf, strlen(buf));
    }
    rio_writen(listenfd, buf, strlen(buf));*/
    while((numberOfBytes = rio_readnb(&rio,buf, MAX_OBJECT_SIZE))) {
        rio_writen(listenfd, buf, numberOfBytes);
        cumulative_bytes += numberOfBytes;
    }
    if(cumulative_bytes <= MAX_OBJECT_SIZE && cumulative_bytes + getCacheSize(cache) <= MAX_CACHE_SIZE) {
        write_cache(cache,hostname, shortUri, buf,cumulative_bytes);
    }
    else {
        free(buf);
    }

    return;
}

/* $end read_requesthdrs */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor="
                 "ffffff"
                 ">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */

//parser uses strstr to locate the location of the host name and port
void parseUrl(char *url, char *hostname, char *port)
{
    char buf[MAXLINE];
    strcpy(buf, url);
    char *p1, *p2;
    p1 = strstr(buf, "//");
    if (p1 == NULL)
    {
        p1 = buf;
    }
    else
    {
        p1 += 2;
    }
    p2 = strstr(p1, ":");
    if (p2 == NULL)
    {
        strcpy(port, "80");
        p2 = strstr(p1, "/");
        if (p2 == NULL)
        {
            p2 = buf + strlen(url);
        }
        *p2 = '\0';
        strcpy(hostname, p1);
    }
    else
    {
        *p2 = '\0';
        strcpy(hostname, p1);
        p1 = p2 + 1;
        p2 = strstr(p2 + 1, "/");
        if (p2 == NULL)
        {
            p2 = buf + strlen(url);
        }
        *p2 = '\0';
        strcpy(port, p1);
    }

    strcpy(buf, url);
    p1 = strstr(buf, "//");
    if (p1 == NULL)
    {
        p1 = buf;
    }
    else
    {
        p1 += 2;
    }
    p2 = strstr(p1, "/");
    if (p2 == NULL)
    {
        strcpy(url, "/");
    }
    else
    {
        strcpy(url, p2);
    }
    printf("url = %s\n", url);
}

void* thread(void * vargp) {
    pthread_detach(pthread_self());
    while(1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        close(connfd);
    }
}

void sigpipe_handler(int sig) {
    printf("SIGPIPE handled\n");
    return;
}