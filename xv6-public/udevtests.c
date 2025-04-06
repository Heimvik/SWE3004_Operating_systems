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
	ps(0);
	setnice(3,0);
	setnice(2,39);
	ps(0);
	printf(1,"DONE\n");
}

int main(int argc, char *argv[]){
    //testsyscalls();
	testsched();
    exit();
}
