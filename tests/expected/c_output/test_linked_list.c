#include "runtime.h"
#include<stdlib.h>
#include <string.h>
#include <stdio.h>

#define NIL 0








struct vtable {
};


typedef struct link {
	struct link* next;
	int value;
} link;

bool is_nil(struct link* x){
        return x == NULL;
        }

struct link* cons(int car, struct link* cdr){
        struct link* new_link = runtime_alloc(sizeof(struct link));
                new_link->next = cdr;
        new_link->value = car;
                        return new_link;
                }

void print_list(struct link* x){
                printf("[");
        while (        is_nil(x) != true) {
{
                        printf("%d", x->value);
                                    x = x->next;
            if (            is_nil(x) != true) {
                        printf(", ");
}
            }
}
                printf("]\n");
        }

int length(struct link* x, int acc){
        if (        is_nil(x)) {
        return acc;
}
        else {
        return         length(x->next, acc + 1);
}
        }

struct link* reverse(struct link* x, struct link* acc){
        if (        is_nil(x)) {
                return acc;
}
        else {
        return         reverse(x->next,         cons(x->value, acc));
}
        }

int main_(){
        struct link* my_list1 =         cons(1, NIL);
                struct link* my_list2 =         cons(2, my_list1);
        ;
                        print_list(my_list2);
        ;
                        }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
