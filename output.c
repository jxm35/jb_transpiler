#include "runtime.h"
#include<stdio.h>

typedef struct Node {
    int data;
}
 Node;

int main_(){
Node* n = runtime_alloc(sizeof(Node));
n->data = 5;
printf("data: %d\n", n->data);
runtime_scope_end();
}

int main() {
    runtime_init();
    main_();
    runtime_print_stats();
}
