#include "types.h"
#include "user.h"

void testsyscalls(){
	printf(1,"PS TEST\n");
	for(int i = 0; i < 5; i++){
		int pid = fork();
		if(pid == 0){
			printf(1,"Child %d doing ps(%d)\n", i,i);
			ps(i);
			exit();
		}
		wait();
	}
	printf(1,"SETNICE TEST\n");
	int validnice = 21;
	int invalidnice_h = 100;
	int invalidnice_l = -1;
	for(int i = 0; i < 5; i++){
		int pid = fork();
		if(pid == 0){
			if(i == 2){
				printf(1,"Child setting nice of PID %d to %d\n", i, invalidnice_h);
				printf(1,"Returns:%d",setnice(i,invalidnice_h));
			}
			else if(i == 3){
				printf(1,"Child setting nice of PID %d to %d\n", i, invalidnice_l);
				printf(1,"Returns:%d",setnice(i,invalidnice_l));
			} else {
				printf(1,"Child setting nice of PID %d to %d\n", i, validnice);
				printf(1,"Returns:%d",setnice(i,validnice));
			}
			exit();
		}
		wait();
	}
	printf(1,"GETNICE TEST\n");
	for(int i = 0; i < 5; i++){
		int pid = fork();
		if(pid == 0){
			printf(1,"Child getting nice of PID %d \n", i);
			printf(1,"Returns:%d",getnice(i));
			exit();
		}
		wait();
	}
	printf(1,"DONE\n");
	exit();
}

int main(int argc, char *argv[]){
    testsyscalls();
    exit();
}
