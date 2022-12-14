#include "../csapp.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);

    if ((buf = getenv("QUERY_STRING")) != NULL) {
    //sprintf(content, "%s argv Slising \r\n<p>", content);

        p = strchr(buf, '&'); // strchr : 문자열 내에 일치하는 문자가 있는지 검사하는 함수.
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p+1);

        if (strchr(arg1, '=')) {
            p = strchr(arg1, '=');
            *p = '\0';
            strcpy(arg1, p + 1);

            p = strchr(arg2, '=');
            *p = '\0';
            strcpy(arg2, p + 1);
        }

        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

//  sprintf(content, "%s n1 %s n2 %s \r\n<p>", content, arg1, arg2);
    sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1+n2);
    sprintf(content, "%sThanks for visiting!\r\n", content);

    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n"); // 클라이언트 에서 \r\n으로 빈줄을 만들어 주면 그 빈줄을 기점으로 윗 부분이 헤더, 아랫 부분이 바디가 된다.
    printf("%s", content);
    fflush(stdout);
    exit(0);
}