#include "runtime.h"
#include<stdio.h>



struct vtable {
};


typedef struct Node {
	int data;
} Node;

int main_(){
        struct Node* n = runtime_alloc(sizeof(struct Node));
        n->data = 5;
        runtime_inc_ref_count(n, NULL);
        struct Node* x = n;
        runtime_inc_ref_count(n, NULL);
        runtime_dec_ref_count(x, 0);
        x = n;
                printf("data: %d\n", n->data);
        runtime_dec_ref_count(n, 0);
        runtime_dec_ref_count(x, 0);
        }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
