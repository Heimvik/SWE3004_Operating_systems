#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

//Q1 global static variables
int static_variable = 10;

//Q2 global static variables

int q1(void){
    int* heap_variable = (int*)malloc(sizeof(int));
    *heap_variable = 10;
    int stack_variable = 10;
    int pid = fork();
    if(pid>0){
        printf("The parent process entered increment\n");
        for(int j = 0; j<5;j++){
            printf("Mother inc: %d %d %d\n",stack_variable++, (*heap_variable)++, static_variable++);
        }
        wait(NULL);
        return 0;
    } else {
        printf("The child process entered increment\n");
        for(int j = 0; j<5;j++){
            printf("Child inc: %d %d %d\n", stack_variable--, (*heap_variable)--,static_variable--);
        }
        return 0;
    }
    return 1;
}

int q2(void){
    //Open a file in the same file directory as the current one
    int fd = open("testFile.txt",O_RDWR|O_CREAT,0666); //READ/WRITE access, create the file upon no find, everyone can access
    pid_t pid = fork();
    if(pid>0){
        const char* parent_msg = "parentwrittentofile\n";
        write(fd,parent_msg,sizeof(char)*strlen(parent_msg));
        if(close(fd)){
            printf("Parent could not close file!");
        }
        wait(NULL);
    } else {
        const char* child_msg = "CHILDWRITTENTOFILE\n";
        write(fd,child_msg,sizeof(char)*strlen(child_msg));
        if(close(fd)){
            printf("Child could not close file!");
        }
    }
    return 0;
}

int childPrinted = 0;

void childHandler(){
    printf("Goodbye from child!\n");
    childPrinted = 1;
}

int q3(void){
    //We now want to do the writing to terminal, and sychronize hello and goodbye between parent/child, without wait()
    pid_t pid = fork();
    if(pid > 0){
        //Parent process
        printf("Hello from parent!\n");
        kill(pid,SIGUSR1);

    } else {
        signal(SIGUSR1,childHandler);
        while(!childPrinted);
    }
    return 0;
}

int q4(void){
    pid_t pid = fork();
    if(pid > 0){
        //Parent process
        wait(NULL);
        printf("Child process terminated");

    } else {
        //Child process
        const char* path = "/bin/ls";
        execl(path, "ls",NULL);

    }
    return 0;
}

int q5(void){
    pid_t pid = fork();
    if(pid > 0){
        //Parent process
        printf("Parent wait returned: %d\n",wait(NULL));
        printf("Child process terminated");
        exit(0);

    } else {
        //Child process
        printf("Child wait returned: %d\n",wait(NULL));
        exit(0);
    }
    return 0;
}


int main(int argc, char* argv[]){
    return q5();
}
