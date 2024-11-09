#include "runtime.h"
#include<stdlib.h>
#include <string.h>
#include <stdio.h>

#define NIL 0

typedef struct link{
	void*	next;
	int	value;
}
 link;

bool is_nil(link* x){
    return x == NULL;
    }

link* cons(int car, link* cdr){
    link* new_link = runtime_alloc(sizeof(link));
        runtime_inc_ref_count(cdr, new_link);
new_link->next = cdr;
    new_link->value = car;
    runtime_inc_ref_count(new_link, NULL);
    runtime_dec_ref_count(new_link, 0);
    return new_link;
    runtime_dec_ref_count(new_link, 0);
    }

void print_list(link* x){
        printf("[");
    while (    is_nil(x) != true) {
{
                printf("%d", x->value);
                runtime_inc_ref_count(x, NULL);
        runtime_dec_ref_count(x, 0);
x = x->next;
        if (        is_nil(x) != true) {
                printf(", ");
}
        }
}
        printf("]\n");
    }

int length(link* x, int acc){
    if (    is_nil(x)) {
    return acc;
}
    else {
    return     length(x->next, acc + 1);
}
    }

link* reverse(link* x, link* acc){
    if (    is_nil(x)) {
    runtime_inc_ref_count(acc, NULL);
    return acc;
}
    else {
    return     reverse(x->next,     cons(x->value, acc));
}
    }

int main_(){
    link* my_list1 =     cons(1, NIL);
    runtime_inc_ref_count(my_list1, NULL);
    link* my_list2 =     cons(2, my_list1);
    runtime_dec_ref_count(my_list1, 0);
;
        runtime_inc_ref_count(my_list2, NULL);
    print_list(my_list2);
    runtime_dec_ref_count(my_list2, 0);
;
    runtime_dec_ref_count(my_list1, 0);
    runtime_dec_ref_count(my_list2, 0);
    }

int main() {
    runtime_init();
    main_();
    runtime_print_stats();
}
