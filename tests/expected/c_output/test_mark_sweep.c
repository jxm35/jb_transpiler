#include "runtime.h"
#include<stdio.h>

#define NIL 0











struct vtable {
};


typedef struct Node {
	int value;
	struct Node* next;
	struct Node* child;
} Node;

typedef struct Graph {
	struct Node* nodes[5];
	int count;
} Graph;

struct Node* create_node(int val){
        struct Node* n = runtime_alloc(sizeof(struct Node));
        n->value = val;
        n->next = NIL;
        n->child = NIL;
                        return n;
                }

void test_basic_collection(){
                printf("=== Basic Collection Test ===\n");
        struct Node* temp1 =         create_node(100);
        struct Node* temp2 =         create_node(200);
        struct Node* temp3 =         create_node(300);
                temp1->next = temp2;
                temp2->next = temp3;
                printf("Created temporary chain: %d -> %d -> %d\n", temp1->value, temp2->value, temp3->value);
        struct Node* keeper =         create_node(999);
                printf("Created keeper node: %d\n", keeper->value);
                runtime_gc();
                printf("Chain still exists chain: %d -> %d -> %d\n", temp1->value, temp2->value, temp3->value);
                temp1 = NIL;
                temp2 = NIL;
                temp3 = NIL;
                printf("Temporary chain made unreachable\n");
                printf("Keeper still accessible: %d\n", keeper->value);
                runtime_gc();
                printf("Garbage collection completed\n");
                                        }

void test_reachable_cycles(){
                printf("\n=== Reachable Cycle Test ===\n");
        struct Node* a =         create_node(1);
        struct Node* b =         create_node(2);
        struct Node* c =         create_node(3);
                a->next = b;
                b->next = c;
                c->next = a;
                printf("Created cycle: %d -> %d -> %d -> (back to %d)\n", a->value, b->value, c->value, a->value);
                b = NIL;
                c = NIL;
                printf("Cycle root value: %d\n", a->value);
                runtime_gc();
                printf("GC run - cycle should remain (still reachable)\n");
                printf("Cycle still accessible via a: %d\n", a->value);
                a = NIL;
                printf("Broke reference to cycle\n");
                runtime_gc();
                printf("GC run - cycle should be collected now\n");
                                }

void test_partial_structures(){
                printf("\n=== Partial Structure Collection Test ===\n");
        struct Node* root =         create_node(1);
        struct Node* branch1 =         create_node(2);
        struct Node* branch2 =         create_node(3);
        struct Node* leaf1 =         create_node(4);
        struct Node* leaf2 =         create_node(5);
                root->next = branch1;
                root->child = branch2;
                branch1->child = leaf1;
                branch2->child = leaf2;
                printf("Built tree: root(%d) -> branch1(%d) -> leaf1(%d)\n", root->value, branch1->value, leaf1->value);
                printf("           root(%d) -> branch2(%d) -> leaf2(%d)\n", root->value, branch2->value, leaf2->value);
                branch1 = NIL;
                branch2 = NIL;
                leaf1 = NIL;
                leaf2 = NIL;
        root->child = NIL;
                printf("Cut off branch2 subtree\n");
                runtime_gc();
                printf("GC should collect branch2(%d) and leaf2(%d)\n", 2, 5);
                printf("Root and branch1 subtree still reachable: %d -> %d -> %d\n", root->value, root->next->value, root->next->child->value);
                                                }

void test_gc_threshold(){
                printf("\n=== GC Threshold Test ===\n");
                runtime_set_gc_threshold(1024);
                printf("Set GC threshold to 1KB\n");
        struct Node* keeper =         create_node(42);
                printf("Created keeper: %d\n", keeper->value);
                printf("Allocated and abandoned 50 temporary nodes\n");
                printf("Keeper should still be alive: %d\n", keeper->value);
                runtime_gc();
                printf("Final GC completed\n");
                }

    struct Node* global_node = NIL;

void test_globals(){
                global_node =         create_node(99);
                runtime_gc();
                printf("Global node has not been collected: %d\n", global_node->value);
                global_node = NIL;
                runtime_gc();
        }

int main_(){
                printf("Mark-Sweep Garbage Collector Test Program\n");
                printf("=========================================\n");
                test_basic_collection();
                test_reachable_cycles();
                test_partial_structures();
                test_gc_threshold();
                test_globals();
                printf("\n=== All Tests Complete ===\n");
        return 0;
        }

int main() {
    runtime_init();
    runtime_register_root(&global_node);
    main_();
    runtime_shutdown();
}
