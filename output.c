#include "runtime.h"
#include<stdio.h>

#define NIL 0

typedef struct Node{
	int	value;
	void*	child;
}
 Node;

typedef struct Container{
	Node*	primary;
	Node*	backup;
}
 Container;

Node* create_node(int val){
    Node* n = runtime_alloc(sizeof(Node));
    n->value = val;
    n->child = NIL;
            return n;
        }

void test_basic_sharing(){
        printf("=== Basic Pointer Sharing Test ===\n");
    Node* shared =     create_node(42);
        Node* ref1 = shared;
        Node* ref2 = shared;
        printf("Shared node value: %d\n", shared->value);
        printf("Ref1 value: %d\n", ref1->value);
        printf("Ref2 value: %d\n", ref2->value);
        ref1 =     create_node(100);
        printf("After reassignment, ref1 value: %d\n", ref1->value);
        printf("Shared still: %d\n", shared->value);
                }

void test_nested_structures(){
        printf("\n=== Nested Structure Test ===\n");
    Node* parent =     create_node(1);
    Node* child =     create_node(2);
        parent->child = child;
        printf("Parent: %d, Child: %d\n", parent->value, parent->child->value);
    Node* parent2 =     create_node(3);
        parent2->child = child;
        printf("Parent2: %d, Shared child: %d\n", parent2->value, parent2->child->value);
    parent->child = NIL;
        printf("After nullifying parent->child, parent2 child still: %d\n", parent2->child->value);
                }

void test_container_patterns(){
        printf("\n=== Container Pattern Test ===\n");
    Container* box = runtime_alloc(sizeof(Container));
    Node* important_data =     create_node(999);
        box->primary = important_data;
        box->backup = important_data;
        printf("Primary: %d, Backup: %d\n", box->primary->value, box->backup->value);
    box->primary = NIL;
        printf("After removing primary, backup still: %d\n", box->backup->value);
    box->primary =     create_node(777);
        printf("New primary: %d, Backup: %d\n", box->primary->value, box->backup->value);
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
    runtime_print_stats();
}
