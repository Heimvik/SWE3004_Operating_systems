#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>


void useMemory(size_t size, int duration){
    uint8_t* memory = (char*)malloc(size);
    printf("Allocated %d bytes\n",size);
    time_t startTime = time(NULL);
    while(1){
        time_t currentTime = time(NULL);
        if(difftime(currentTime,startTime)>= duration){
            free(memory);
            printf("Freed memory\n");
            break;
        } else {
            for(size_t cur = 0;cur<size;cur++){
                memory[cur] ^= memory[cur]+1;
            }
        }
    }
}

int main(char argc, char* argv[]){
    if(argc!=3){
        printf("Usage: %s <size_t_value> <int_value>\n", argv[0]);
    } else {
        useMemory((size_t)strtoull(argv[1], NULL, 10),atoi(argv[2]));
    }
    useMemory(0,atoi(argv[2]));
    return 0;
}