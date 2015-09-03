#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int pid;
    extern void docommand(char* out);

/* если параметры не заданы, то вывод осуществляется в файл a.txt */

    char* out = "a.txt";        
    if(argc == 2)
      out = argv[1];

    printf("Executing 'ls -lisa | sort | wc -l'\n");
    fflush(stdout);  

    /* создание первого процесса потомка - wc */
    switch ((pid = fork())) {
    case -1:/* произошла ошибка */
        perror("fork");
        break;
    case 0:/* процесс потомок - wc */
        docommand(out);
        break; 
    default:
        /* parent; fork() return value is child pid */

        pid = waitpid(pid,0,0);
    }
    return(0);
}

/*Выполняет команду 'ls -lisa | sort | wc -l', выводит результат в файл с именем out */
void docommand(char* out)  
{
    int pipefd[2];

    /* создание канала wc - sort */ 
    if (pipe(pipefd)) 
    {
        perror("pipe");
        exit(127);
    }
    /* создание второго процесса потомка - sort */
    switch (fork()) {
    case -1:/* произошла ошибка */
        perror("fork");
        exit(127);
    case 0:/* процесс потомок - sort */
        {

            /* создание канала sort - ls */ 
            int pipefd2[2];
            if (pipe(pipefd2)) 
            {
                perror("pipe");
                exit(127);
            }

            /* создание третьего процесса потомка - ls */
            switch (fork()) {
            case -1:/* произошла ошибка */
                perror("fork");
                exit(127);
            case 0:/* процесс потомок - ls */

                /* перенаправление вывода в канал */            
                close(pipefd2[0]);  
                dup2(pipefd2[1], 1);  
                close(pipefd2[1]);  

                /* exec ls */
                execlp("ls", "ls", "-lisa", (char *)NULL);
                perror("/usr/bin/ls");
                exit(126);

            default:/* процесс потомок - sort */

                /* перенаправление вывода в канал */        
                close(pipefd[0]);       
                dup2(pipefd[1], 1);     
                close(pipefd[1]);       

                /* перенаправление ввода на ввод из канала канал */ 
                close(pipefd2[1]);
                dup2(pipefd2[0], 0);  
                close(pipefd2[0]);

                /* exec sort */
                execlp("sort", "sort", (char *)NULL);
                            
                perror("sort");
                exit(126);
            }
        }

    default:/* процесс потомок - wc */
       
        /* перенаправление вводы на ввод из канала канал */ 
        close(pipefd[1]);       
        dup2(pipefd[0], 0);     
        close(pipefd[0]);       

        /*перенаправление вывода на вывод в файл */
        int fd3;
        fd3 = open(out, O_RDWR | O_CREAT,S_IRWXU);
        close(1);
        fcntl(fd3, F_DUPFD, 1);

        /* exec wc */
        execlp("wc", "wc","-l", (char *)NULL);
        perror("/bin/wc");
        exit(125);
    }
}

