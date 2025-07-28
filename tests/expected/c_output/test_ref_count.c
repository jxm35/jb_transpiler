#include "runtime.h"
#include<stdio.h>

#define NIL 0








struct vtable {
};


typedef struct Node {
	int value;
	struct Node* child;
} Node;

typedef struct Container {
	struct Node* primary;
	struct Node* backup;
} Container;

struct Node* create_node(int val){
        struct Node* n = runtime_alloc(sizeof(struct Node));
        n->value = val;
        n->child = NIL;
        runtime_inc_ref_count(n, NULL);
        runtime_dec_ref_count(n, 0);
        return n;
        runtime_dec_ref_count(n, 0);
        }

void test_basic_sharing(){
                printf("=== Basic Pointer Sharing Test ===\n");
        struct Node* shared =         create_node(42);
        runtime_inc_ref_count(shared, NULL);
        struct Node* ref1 = shared;
        runtime_inc_ref_count(shared, NULL);
        struct Node* ref2 = shared;
                printf("Shared node value: %d\n", shared->value);
                printf("Ref1 value: %d\n", ref1->value);
                printf("Ref2 value: %d\n", ref2->value);
        runtime_dec_ref_count(ref1, 0);
        ref1 =         create_node(100);
                printf("After reassignment, ref1 value: %d\n", ref1->value);
                printf("Shared still: %d\n", shared->value);
        runtime_dec_ref_count(ref1, 0);
        runtime_dec_ref_count(ref2, 0);
        runtime_dec_ref_count(shared, 0);
        }

void test_nested_structures(){
                printf("\n=== Nested Structure Test ===\n");
        struct Node* parent =         create_node(1);
        struct Node* child =         create_node(2);
        runtime_inc_ref_count(child, parent);
        parent->child = child;
                printf("Parent: %d, Child: %d\n", parent->value, parent->child->value);
        struct Node* parent2 =         create_node(3);
        runtime_inc_ref_count(child, parent2);
        parent2->child = child;
                printf("Parent2: %d, Shared child: %d\n", parent2->value, parent2->child->value);
        parent->child = NIL;
                printf("After nullifying parent->child, parent2 child still: %d\n", parent2->child->value);
        runtime_dec_ref_count(child, 0);
        runtime_dec_ref_count(parent, 0);
        runtime_dec_ref_count(parent2, 0);
        }

void test_container_patterns(){
                printf("\n=== Container Pattern Test ===\n");
        struct Container* box = runtime_alloc(sizeof(struct Container));
        struct Node* important_data =         create_node(999);
        runtime_inc_ref_count(important_data, box);
        box->primary = important_data;
        runtime_inc_ref_count(important_data, box);
        box->backup = important_data;
                printf("Primary: %d, Backup: %d\n", box->primary->value, box->backup->value);
        box->primary = NIL;
                printf("After removing primary, backup still: %d\n", box->backup->value);
        box->primary =         create_node(777);
                printf("New primary: %d, Backup: %d\n", box->primary->value, box->backup->value);
        runtime_dec_ref_count(box, 0);
        runtime_dec_ref_count(important_data, 0);
        }

int main_(){
                printf("Reference Counting Test Program\n");
                printf("===============================\n");
                test_basic_sharing();
                test_nested_structures();
                test_container_patterns();
                printf("\n=== Test Complete ===\n");
        return 0;
        }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
