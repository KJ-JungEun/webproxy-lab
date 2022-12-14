/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
int parse_uri(char *uri, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void read_requesthdrs(rio_t *rp);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void echo(int connfd);
void handler(int sig);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  if(argc !=2) {
    fprintf(stderr, "usage: %s, <port>\n", argv[0]);
    exit(1);
  }
  printf("---------------\n");
  listenfd = Open_listenfd(argv[1]);
  printf("open-ip : 43.200.180.240\n");
  printf("open-port : %s\n", argv[1]);
  printf("---------------\n");
  while(1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);
    //echo(connfd);
    Close(connfd);
    printf("===============================================\n");
  }
}

void doit(int fd) {
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if(!(strcasecmp(method, "GET")==0 || strcasecmp(method, "HEAD")==0)) { //if method != get, head
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio);

  is_static = parse_uri(uri, filename, cgiargs);
  printf("uri : %s, filename : %s, cgiargs : %s \n", uri, filename, cgiargs);

  if(stat(filename, &sbuf) < 0) { //can't find file
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if(is_static) { //static_service
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //no authority
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
  }
  serve_static(fd, filename, sbuf.st_size, method);
  }
  else { //dynamic_service
     if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //no authority
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
     }
     serve_dynamic(fd, filename, cgiargs);
  }
}

int parse_uri(char *uri, char *filename, char *cgiargs) {
  char *ptr;
  if(!strstr(uri, "cgi-bin")) { //no cgi-vin => static content
    strcpy(cgiargs, ""); //remove cgi-args
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri) - 1] == '/') {
      strcat(filename, "home.html");
    }
    return 1;
  }
  else { //dynamic content
    // ex :  http://43.200.180.240:80/cgi-bin/adder?123&456
    ptr = index(uri, '?');
    if(ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  // Build the HTTP response body 
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // Print the HTTP response
  sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {  
    Rio_readlineb(rp, buf, MAXLINE);
  }
  return;
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
  char buf[MAXLINE], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server : Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0) {
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);
    Execve(filename, emptylist, environ);
  }
  //Wait(NULL);
  handler(NULL);
}

void serve_static(int fd, char *filename, int filesize, char *method) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXLINE];
  get_filetype(filename, filetype);

  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);
  sprintf(buf, "%sConnection : close \r\n", buf);
  sprintf(buf, "%sConnect-length : %d \r\n", buf, filesize);
  sprintf(buf, "%sContent-type : %s \r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers : \n");
  printf("%s", buf);

  if(!(strcasecmp(method, "GET"))) {
    srcfd = Open(filename, O_RDONLY, 0);

  /*
  homework_exam 11.9
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  Munmpa(srcp, filesize)
  */
 
  srcp = (char *)Malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);
  Rio_writen(fd, srcp, filesize);
  free(srcp);
  }
}

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filename, "image/.png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg"); 
    else if (strstr(filename, ".mp4")) //homework_exam 11.7
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void echo(int connfd){
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        if(strcmp(buf, "\r\n") == 0)
            break;
        Rio_writen(connfd, buf, n);
    }
}

void handler(int sig) {
  int olderrno = errno;

  while(waitpid(-1, NULL, 0) > 0) {
    printf("cleaning child process\n");
  }
  if(errno != ECHILD) {
    Sio_error("waitpid error");
  }
  Sleep(1);
  errno = olderrno;
}