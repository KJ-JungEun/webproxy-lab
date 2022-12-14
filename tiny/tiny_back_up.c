#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filenmae, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);
void echo(int connfd);

/**
 *	@fn		main
 *
 *	@brief                      Tiny main 루틴, TINY는 반복실행 서버로 명령줄에서 넘겨받은
 *                              포트로의 연결 요청을 듣는다. open_listenfd 함수를 호출해서
 *                              듣기 소켓을 오픈한 후에, TINY는 전형적인 무한 서버 루프를
 *                              실행하고, 반복적으로 연결 요청을 접수하고, 트랙잭션을
 *                              수행하고, 자신 쪽의 연결 끝을 닫는다.
 *
 *  @param  int argc
 *  @param  char **argv
 *
 *	@return	int
 */


int main(int argc, char **argv) {
    // fd : 파일 또는 소켓을 지칭하기 위해 부여한 숫자
    int listenfd, connfd; // 듣기 식별자, 연결 식별자를 선언 
    char hostname[MAXLINE], port[MAXLINE]; // 호스트 이름과 포트를 저장할 캐릭터 배열 선언
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    printf("-----------------\n");
    printf("host name : %s\n", hostname);
    printf("port num  : %s\n", port);
    listenfd = Open_listenfd(argv[1]);
    printf("new host name : %s\n", hostname);
    printf("new port num  : %s\n", port);
    while (1) { // 무한 서버 루프

        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen); // 반복적으로 연결 요청을 접수
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        // printf("-----------------\n");
        // printf("host name : %s\n", hostname);
        // printf("port num  : %s\n", port);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd); // 트랜잭션 수행  , 연습문제 11.6 할때 잠깐 comment 해줌
        // echo(connfd); // 연습문제 11.6
        Close(connfd); // 자신 쪽의 연결 끝을 닫는다.
        printf("===============================================\n\n");

    }
}

/**
 *	@fn		doit
 *
 *	@brief                      한 개의 HTTP 트랜잭션을 처리한다. 먼저, 요청 라인을 읽고 분석한다.
 *                              포트로의 연결 요청을 듣는다. open_listenfd 함수를 호출해서
 *                              듣기 소켓을 오픈한 후에, TINY는 전형적인 무한 서버 루프를
 *                              실행하고, 반복적으로 연결 요청을 접수하고, 트랙잭션을
 *                              수행하고, 자신 쪽의 연결 끝을 닫는다.
 *                        
 *
 *  @param  int fd
 *
 *	@return	void
 */
void doit(int fd) { // handle one HTTP request/response transaction
    int is_static;
    struct stat sbuf;
    // MAXLINE = Max text line length
    // 여러가지 정보를 담을 캐릭터 변수를 선언
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];

    // rio 구조체 선언
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd); // &rio 주소를 가지는 읽기 버퍼를 만들고 초기화 한다. (rio_t 구조체 초기화)
    Rio_readlineb(&rio, buf, MAXLINE); // 버퍼에서 읽은 것이 담겨있다. 
    // rio_readlineb함수를 사용해서 요청 라인을 읽어 들인다.
    
    printf("Request headers:\n"); // 요청라인을 읽고
    printf("%s", buf); // "GET / HTTP/1.1"
    sscanf(buf, "%s %s %s", method, uri, version); // 버퍼에서 자료형을 읽는다, 요청라인을 분석한다.

    // TINY는 GET 메소드만 지원한다. 만일 클라이언트가 다른 메소드(POST 같은)를 요청하면, 에러 메시지를 보내고
    // main 루틴으로 돌아오고 그 후에 연결을 닫고 다음 연결 요청을 기다린다.
    // 그렇지 않으면 밑에 read_requesthdrs에서 읽어들이고 다른 요청 헤더들은 무시한다.
    if(strcasecmp(method, "GET")){ // strcasecmp : 두 문자열의 길이와 내용이 같을때 0을 반환
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }


    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    // uri를 분석한다.
    // 파일이 없는 경우 에러를 띄운다/
    // parse_uri를 들어가기 전에 filename과 cgiargs는 없다.
    // 이 URI를 CGI 인자 스트링으로 분석하고 요청이 정적 또는 동적 컨텐츠인지 flag를 설정한다.
    is_static = parse_uri(uri, filename, cgiargs);
    printf("uri : %s, filename : %s, cgiargs : %s \n", uri, filename, cgiargs);

    // 만일 이 파일이 디스크 상에 있지 않으면, 에러 메시지를 즉시 클라이언트에게 보내고 리턴한다.
    if (stat(filename, &sbuf) < 0) { //stat는 파일 정보를 불러오고 sbuf에 내용을 적어준다. ok 0, errer -1
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    // 정적 콘텐츠일경우
    if (is_static) { /* Serve static content */
        //파일 읽기 권한이 있는지 확인하기
        //S_ISREG : 일반 파일인가? , S_IRUSR: 읽기 권한이 있는지? S_IXUSR 실행권한이 있는가?
        // 우리는 이 파일이 보통 파일이라는 것과 읽기 권한을 가지고 있는지를 검증한다.
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            //권한이 없다면 클라이언트에게 에러를 전달
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        //그렇다면 클라이언트에게 파일 제공
        serve_static(fd, filename, sbuf.st_size);
    }//정적 컨텐츠가 아닐경우
    else { /* Serve dynamic content */
        // 파일이 실행가능한 것인지 검증한다.
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            //실행이 불가능하다면 에러를 전달
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        //그렇다면 클라이언트에게 파일 제공.
        serve_dynamic(fd, filename, cgiargs);
    }
}

/**
 *	@fn		parse_uri
 *
 *	@brief                      TINY는 정적 컨텐츠를 위한 홈 디렉토리가 자신의 현재 디렉토리고, 
 *                              실행파일의 홈 디렉토리는 /cgi-bin이라고 가정한다. 스트링 cgi-bin을 포함하는
 *                              모든 URI는 동적 컨텐츠를 요청하는 것을 나타낸다고 가정한다. 기본 파일 이름은 ./home.html이다.
 *                              
 *                        
 *  @param  char* uri
 *  @param  char* filename
 *  @param  char* cgiargs
 *
 *	@return	int
 */
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    // URI를 파일 이름과 옵션으로 CGI 인자 스트링을 분석한다.
    // cgi-bin이 없다면 (즉 정적 컨텐츠를 위한 것이라면)
    if (!strstr(uri, "cgi-bin")) { /* Static content*/
        strcpy(cgiargs, ""); // cgi 인자 스트링을 지운다.
        strcpy(filename, "."); 
        strcat(filename, uri); // ./home.html 이 된다 (상대 리눅스 경로 이름으로 변환함)
        if (uri[strlen(uri) - 1] == '/') // 만일 URI가 '/' 문자로 끝난다면
            strcat(filename, "home.html"); // 기본 파일 이름을 추가한다.
        return 1;
    } else { /* Dynamic content*/ // 만약 동적 컨텐츠를 위한 것이라면
        //http://15.165.159.206:1024/cgi-bin/adder?123&456
        ptr = index(uri, '?');
        // 모든 CGI인자 추출한다.
        if (ptr) {
            // 물음표 뒤에 있는 인자 다 갖다 붙인다.
            strcpy(cgiargs, ptr + 1);
            //포인터는 문자열 마지막으로 바꾼다.
            *ptr = '\0'; // uri물음표 뒤 다 없애기
        } else
        
        strcpy(cgiargs, ""); // 물음표 뒤 인자들 전부 넣기
        strcpy(filename, "."); // 나머지 부분 상대 리눅스 uri로 바꿈,
        strcat(filename, uri); // ./uri 가 된다.
        return 0;
    }
}


/**
 *	@fn		clienterror
 *
 *	@brief                      클라이언트에게 오류 보고 한다. TINY는 실제 서버에서 볼 수 있는 많은 에러 처리 기능들이 빠져 있다.
 *                              그러나 일부 명백한 오류에 대해서는 체크하고 있으며, 이들을 클라이언트에게 보고한다.
 *                              이 함수는 HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 클라이언트에 보내며,
 *                              브라우저 사용자에게 에러를 설명하는 응답 본체에 HTML 파일도 함께 보낸다. 
 *                              HTML 응답은 본체에서 컨텐츠의 크기와 타입을 나타내야 한다. 그래서 HTML 컨텐츠를 한 개의 스트링으로 만들었다.
 *                              이로 인해 그 크기를 쉽게 결정할 수 있다. 이 함수에서는 ROBUST한 rio_writen 함수를 모든 출력에 대해 사용하고있다.
 *                        
 *
 *  @param  int fd
 *  @param  char* cause
 *  @param  char* errnum
 *  @param  char* shortmsg
 *  @param  char* longmsg
 *
 *	@return	void
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


/**
 *	@fn		read_requesthdrs
 *
 *	@brief                      요청 헤더를 읽는다. TINY는 요청 헤더 내의 어떤 정보도 사용하지 않는다.
 *                              단순히 read_requesthdrs함수를 호출해서 이들을 읽고 무시한다. 
 *                        
 *
 *  @param  rio_t *rp
 *
 *	@return	void
 */
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE); // 버터에서 MAXLINE 까지 읽기
    while (strcmp(buf, "\r\n")) {  
    // 끝줄 나올 때 까지 읽는다. 요청 헤더를 종료하는 빈 텍스트 줄이 위 line에서 체크하고 있는 
    // carriage return과 line feed 쌍으로 구성되어 있다.
    
        Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}



/**
 *	@fn		serve_dynamic
 *
 *	@brief                      TINY는 자식 프로세스를 fork하고, 그 후에 CGI 프로그램을 자식의 컨텍스트에서
 *                              실행하며모든 종류의 동적 컨텐츠를 제공한다. 이 함수는 클라이언트에 성공을 알려주는
 *                              응답 라인을 보내는 것으로 시작된다. CGI 프로그램은 응답의 나머지 부분을 보내야 한다.
 *                              이것은 우리가 기대하는 것만큼 견고하지 않은데, 그 이유는 이것이 CGI 프로그램이 에러를
 *                              만날 수 있다는 가능성을 염두에 두지 않았기 때문이다.
 *
 *  @param  int fd              
 *  @param  char* filename
 *  @param  char* cgiargs
 *
 *	@return	void
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf)); 
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    
    //클라이언트는 성공을 알려주는 응답라인을 보내는 것으로 시작한다.
    if (Fork() == 0) { //타이니는 새로운 자식프로세스를 포크하고 동적 컨텐츠를 제공한다.
        setenv("QUERY_STRING", cgiargs, 1);
        //자식은 QUERY_STRING 환경변수를 요청 uri의 cgi인자로 초기화 한다.  (15000 & 213)
        Dup2(fd, STDOUT_FILENO); //자식은 자식의 표준 출력을 연결 파일 식별자로 재지정하고 (Duplicate),

        Execve(filename, emptylist, environ); // 그 후에 cgi프로그램을 로드하고 실행한다.
        // CGI 프로그램이 자식 컨텍스트에서 실행되기 때문에 execve함수를 호출하기 전에 존재하던 열린 파일들과
        // 환경 변수들에도 동일하게 접근할 수 있다. 그래서 CGI 프로그램이 표준 출력에 쓰는 모든 것은 
        // 직접 클라이언트 프로세스로 부모 프로세스의 어떤 간섭도 없이 전달된다.
    }
    Wait(NULL); //부모는 자식이 종료되어 정리되는 것을 기다린다.
}


/**
 *	@fn		serve_static
 *
 *	@brief                      TINY는 다섯개의 서로 다른 정적 컨텐츠 타입을 지원 한다.
 *                              HTML 파일, 무형식 텍스트 파일, GIF, PNG, JPEG로 인코딩된 영상
 *                              이 함수는 지역 파일의 내용을 포함하고 있는 본체를 갖는 HTTP응답을 보낸다.
 *
 *  @param  int fd              응답받는 소켓(연결식별자), 파일 이름, 파일 사이즈
 *  @param  char* filename
 *  @param  int filesize
 *
 *	@return	void
 */
void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* send response headers to client*/
    //파일 접미어 검사해서 파일이름에서 타입 가지고
    get_filetype(filename, filetype);  // 접미어를 통해 파일 타입 결정한다.

    // 클라이언트에게 응답 줄과 응답 헤더 보낸다.
    // 클라이언트에게 응답 보내기
    // 데이터를 클라이언트로 보내기 전에 버퍼로 임시로 가지고 있는다.
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);
    sprintf(buf, "%sConnection : close \r\n", buf);
    sprintf(buf, "%sConnect-length : %d \r\n", buf, filesize);
    sprintf(buf, "%sContent-type : %s \r\n\r\n", buf, filetype);
    
    Rio_writen(fd, buf, strlen(buf));
    //rio_readn은 fd의 현재 파일 위치에서 메모리 위치 usrbuf로 최대 n바이트를 전송한다.
    //rio_writen은 usrfd에서 식별자 fd로 n바이트를 전송한다.
    // (요청한 파일의 내용을 연결 식별자 fd로 복사해서 응답 본체를 보낸다.)
    //서버에 출력
    printf("Response headers : \n");
    printf("%s", buf);

    //읽을 수 있는 파일로 열기 (읽기 위해서 filename을 오픈하고 식별자를 얻어온다.)
    srcfd = Open(filename, O_RDONLY, 0); //open read only 읽고
    // PROT_READ -> 페이지는 읽을 수만 있다.
    // 파일을 어떤 메모리 공간에 대응시키고 첫주소를 리턴
    
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 메모리로 넘긴다. 리눅스 mmap 함수는 요청한 파일을 가상메모리 영역으로 매핑한다.
    // mmap을 호출하면 파일 srcfd의 첫 번째 filesize 바이트를 주소 srcp에서 시작하는 사적읽기-허용 가상메모리 영역으로 매핑한다.
    // 지금은 숙제 문제 11.9에 의해 주석처리된 상태임.
    
    srcp = (char *) Malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    //매핑위치, 매핑시킬 파일의 길이, 메모리 보호정책, 파일공유정책,srcfd ,매핑할때 메모리 오프셋

    Close(srcfd);// 파일을 메모리로 매핑한 후에 더 이상 이 식별자는 필요없으며, 그래서 이 파일을 닫는다. 
    // 이렇게 하지 않으면 치명적일 수 있는 메모리 누수가 발생할 수 있다.


    Rio_writen(fd, srcp, filesize); //rio_writen함수는 주소 srcp에서 시작하는 filesize 바이트(물론, 이것은 요청한 파일에 매핑되어있다.)를 클라이언트의 연결 식별자로 복사한다.


    // mmap() 으로 만들어진 -맵핑을 제거하기 위한 시스템 호출이다
    // 대응시킨 녀석을 풀어준다. 유효하지 않은 메모리로 만듦

    // void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    // int munmap(void *start, size_t length);
    free(srcp);
//    Munmap(srcp, filesize); //메모리 해제, 매핑된 가상메모리 주소를 반환한다. 이것은 치명적일 수 있는 메모리 누수를 피하는데 중요하다.
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
    // 숙제 문제 11.7
    // TINY를 확장해서 MPG 비디오 파일을 처리하기 위해 추가한다. 
    else if (strstr(filename, ".mp4")) 
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