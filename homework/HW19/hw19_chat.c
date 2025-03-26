#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define PAGESIZE 4096  // Assumed page size (4 KB)
#define NITERATIONS 1e4

// Function to get the current time in microseconds
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

// Function to measure the time it takes to touch NUMPAGES pages
double measure_tlb_cost(int num_pages) {
    int *a;
    long long start_time, end_time;
    double avg_time;

    // Allocate memory for a large array of size PAGESIZE * num_pages
    a = (int*)malloc(PAGESIZE * num_pages);

    if (a == NULL) {
        perror("malloc failed");
        exit(1);
    }

    // Measure the time for the trial loop
    start_time = get_time_us();

    // Perform num_trials iterations, each accessing num_pages
    for (int trial = 0; trial < NITERATIONS; trial++) {
        for (int i = 0; i < num_pages; i++) {
            a[i * PAGESIZE / sizeof(int)] += 1;  // Access a page
        }
    }

    end_time = get_time_us();
    free(a);

    // Calculate the average time per access in microseconds
    avg_time = (double)(end_time - start_time) / (num_pages * NITERATIONS);

    return avg_time;
}

int main(int argc, char *argv[]) {

    int num_pages = atoi(argv[1]);

    // Measure the cost of accessing the pages
    double avgTime = measure_tlb_cost(num_pages);

    printf("%f\n", avgTime); //Printf to stdout, let python program pick it up

    return 0;
}
