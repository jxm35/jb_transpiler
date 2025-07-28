#include "runtime.h"
#include<stdio.h>




struct vtable {
};


void add_in_place(int a, int b, int* c){
                *c = a + b;
        }

int add(int a, int b){
        int* c = runtime_alloc(sizeof(int));
                        add_in_place(a, b, c);
        ;
                return *c;
                }

int main_(){
        int x = 5;
        int y = 10;
        int z =         add(x, y);
                printf("Result: %d\n", z);
        }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
