#include "runtime.h"
#include <stdio.h>
#include <stdbool.h>

int add(int a, int b){
return a + b;
}

int main(){
int x = 5;
int y = 10;
int z = add(x, y);
printf("Result: %d\n", z);
}

