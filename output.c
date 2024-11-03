#include "runtime.h"
#include<stdio.h>

#define MAX_SIZE 100

typedef int Integer;

typedef struct Matrix{
	int data[10][10];
}
 Matrix;

void processArray(int numbers[10], int size){
    int i = 0;
    while (i < size) {
        numbers[i] = numbers[i] * 2;
        i = i + 1;
        printf("numbers[i] = %d\n", numbers[i]);
    }
}

int main_(){
    int numbers[100];
    Matrix m;
    m.data[0][0] = 42;
    processArray(numbers, MAX_SIZE);
}

int main() {
    runtime_init();
    main_();
    runtime_print_stats();
}
