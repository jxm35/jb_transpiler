#include "runtime.h"
#include<stdio.h>



struct vtable {
};


typedef struct Point {
	int x;
	int y;
} Point;

int main_(){
        struct Point p;
        struct Point* pp = &p;
        p.x = 10;
        pp->y = 20;
        int* ip = &p.x;
                *ip = 30;
        if (p.x > 20) {
{
            }
}
        else {
{
            }
}
        while (p.x > 0) {
{
                        printf("P.x = %d\n", p.x);
            p.x = p.x - 7;
            }
}
                        }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
