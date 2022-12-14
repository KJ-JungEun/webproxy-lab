#include "csapp.h"

int main(int argc, char **argv) {
    struct addrinfo *p, *listp, hints;
    struct sockaddr_in *sockp; //for using inet_ntop
    char buf[MAXLINE];
    int rc, flags;

    if(argc != 2) {
        fprintf(stderr, "usage : %s <domain name> \n", argv[0]);
        exit(0);
    }

    /* Get a list of addrinfo records */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // Ipv4 only
    hints.ai_socktype = SOCK_STREAM; // connections only
    if((rc = getaddrinfo(argv[1], NULL, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo error : %s \n", gai_strerror(rc));
        exit(1);
    }

    // walk the list and display each ip address (getnameinfo)
    /*
    flags = NI_NUMERICHOST; // display address string instead of domain name
    for (p = listp; p; p=p->ai_next) {
        Getnameinfo(p->ai_addr, p->ai_addrlen, buf, MAXLINE, NULL, 0, flags);
        printf("%s \n", buf);
    }
    */
   
    // walk the list and display each ip address (inet_ntop)
    for (p = listp; p; p=p->ai_next) {
        sockp = (struct sockaddr_in *)p->ai_addr;
        Inet_ntop(AF_INET, &(sockp->sin_addr), buf, MAXLINE);
        printf("%s \n", buf);
    }

    //clean up
    Freeaddrinfo(listp);

    exit(0);    
}