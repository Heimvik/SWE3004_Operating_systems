#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

#define NITERATIONS 1e4

long long getTimeUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

double getAvgPageAccessTime(int numPages){
    //The TLB a given anount of entries for a given anount of pages. As number of pages we access increase, more TLB misses
    size_t pageSize = getpagesize();

    //The number of ints in one page
    size_t jump = pageSize/sizeof(int); 
    
    //volatile to avoid optimaizations
    volatile int* a = (int*)calloc(jump*numPages,sizeof(int)); 
    
    //Time taking defs
    long long start,end;
    double elapsedAvg = 0;

    start = getTimeUs();
    for(int i = 0; i<NITERATIONS;i++){
        for(int i = 0;i<jump*numPages;i+=jump){
            a[i] += 1;
        }
    }
    free((void*)a);
    end = getTimeUs();
    return (double)(end - start) / (numPages * NITERATIONS);
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <numPages>\n", argv[0]);
        return 1;
    }
    int numPages = atoi(argv[1]);
    double avgTime = getAvgPageAccessTime(numPages);
    printf("%f\n", avgTime); //Printf to stdout, let python program pick it up
    return 0;
}