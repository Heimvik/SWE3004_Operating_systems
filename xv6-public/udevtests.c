#include "types.h"
#include "user.h"

void testsyscalls(){
	printf(1,"\nPS TEST\n");
	for(int i = 0; i < 5; i++){
		printf(1,"Doing ps(%d)\n", i,i);
		ps(i);
	}
	printf(1,"\nSETNICE TEST\n");
	int validnice = 21;
	int invalidnice_h = 100;
	int invalidnice_l = -13;
	for(int i = 0; i < 6; i++){
		if(i == 2){
			printf(1,"Setting nice of PID %d to %d\n", i, invalidnice_h);
			printf(1,"Returns:%d\n",setnice(i,invalidnice_h));
		}
		else if(i == 3){
			printf(1,"Setting nice of PID %d to %d\n", i, invalidnice_l);
			printf(1,"Returns:%d\n",setnice(i,invalidnice_l));
		} else {
			printf(1,"Setting nice of PID %d to %d\n", i, validnice);
			printf(1,"Returns:%d\n",setnice(i,validnice));
	        }
	}
	printf(1,"\nGETNICE TEST\n");
	for(int i = 0; i < 6; i++){
		printf(1,"Getting nice of PID %d \n", i);
		printf(1,"Returns:%d\n",getnice(i));
	}
	ps(0);
	printf(1,"DONE\n");
}

void testsched(){
	int pids[5];
	int nicevalues[5] = {0, 10, 20, 30, 39};
	int workload = 1000000; // Adjust workload as needed
	
	for (int i = 0; i < 5; i++) {
		pids[i] = fork();
		if (pids[i] == 0) {
			setnice(getpid(), nicevalues[i]);
			printf(1, "PID %d | nice %d started.\n", getpid(), nicevalues[i]);
			if(i == 4) {
				ps(0);//Pids should be similar here
			}
			volatile int sum = 0;
			for (volatile int j = 0; j < workload; j++) {
				sum += j;
			}
			//printf(1, "PID %d | nice %d finished\n", getpid(), nicevalues[i]);
			exit();
		}
	}
	
	for (int i = 0; i < 5; i++) {
		wait();
	}
	printf(1, "All child processes completed.\n");
	printf(1,"DONE\n");
	ps(0);
}

int main(int argc, char *argv[]){
    //testsyscalls();
	testsched();
    exit();
}
