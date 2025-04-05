#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

void q1()
{
    int *i = NULL;
    int j = *i;
}

void q4()
{
    int *i = (int *)malloc(sizeof(int));
}

void q5()
{
    int *data = (int *)malloc(100 * sizeof(int));
    data[100] = 0;
    free(data);
}

void q6()
{
    int *data = (int *)malloc(10 * sizeof(int));
    free(data+5);
    printf("%d\n", data[0]);
}


typedef struct {
    int *array;
    size_t size;
    size_t capacity;
} fifo_t;

void initFifo(fifo_t *vec) {
    vec->array = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

void pushFifo(fifo_t *vec, int value) {
    if (vec->size == vec->capacity) {
        vec->capacity = (vec->capacity == 0) ? 1 : vec->capacity * 2;
        vec->array = (int *)realloc(vec->array, vec->capacity * sizeof(int));
    }
    vec->array[vec->size++] = value;
}

int popFifo(fifo_t *vec) {
    int value = vec->array[0];
    for (size_t i = 1; i < vec->size; i++) {
        vec->array[i - 1] = vec->array[i];
    }
    vec->size--;
    return value;
}

void freeFifo(fifo_t *vec) {
    free(vec->array);
    vec->array = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

void q8() {
    fifo_t vec;
    initFifo(&vec);

    const int num_elements = 10000;
    clock_t start, end;

    start = clock();
    for (int i = 0; i < num_elements; i++) {
        pushFifo(&vec, i);
    }
    end = clock();
    printf("Time to push %d elements: %f seconds\n", num_elements, (double)(end - start) / CLOCKS_PER_SEC);

    start = clock();
    for (int i = 0; i < num_elements; i++) {
        popFifo(&vec);
    }
    end = clock();
    printf("Time to pop %d elements: %f seconds\n", num_elements, (double)(end - start) / CLOCKS_PER_SEC);

    freeFifo(&vec);
}
int main(int argc, char *argv[])
{
    // q1();
    // q4();
    // q5();
    q8();
    return 0;
}
