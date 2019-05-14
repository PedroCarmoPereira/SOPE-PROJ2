    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <stdlib.h>
    #include <pthread.h>
    #include <limits.h>
    #include <errno.h>
    #include <sys/wait.h>
    #include "string.h"
    #include "../utils/constants.h"
    #include "../utils/types.h"
    #include "../utils/sope.h"
    #include "hashsum.h"

    #define READ 0
    #define WRITE 1
    #define MAXLINE 1000

    void err_sys(char *msg);
    void err_msg(char *msg);

    void hashGenerator(char *salt, char *password){

        int fd1[2], fd2[2];
                pid_t pid;
                char hash[HASH_LEN];

                if (pipe(fd1) < 0 || pipe(fd2) < 0)
                    err_sys("pipe error");
                if ((pid = fork()) < 0)
                    err_sys("fork error");

                if (pid == 0) /* child 1 */{

                    close(fd1[WRITE]);
                    close(fd2[READ]);
                    dup2(fd1[READ], STDIN_FILENO);
                    close(fd1[READ]);
                    dup2(fd2[WRITE], STDOUT_FILENO);
                    close(fd2[WRITE]);

                    execl("echo", "echo", strcat(password, salt), NULL);
                    exit(1);
                }
                else /*parent*/{
                    close(fd1[READ]);
                    close(fd2[WRITE]);

                    write(fd1[WRITE], "This is the data\n", 14);
                    close(fd1[WRITE]);
                    read(fd2[READ], hash, HASH_LEN);

                    waitpid(pid, NULL, 0);
                    close(fd2[READ]);
                }
            }
            
            void err_sys(char *msg){
                fprintf(stderr, "%s\n", msg);
                exit(1);
            }
            void err_msg(char *msg){
                printf("%s\n", msg);
                return;
            }

void testPrint(){

    puts("AAA");
}
