#include "runtime.h"
#include<stdio.h>



struct vtable {
};


struct Point {
    int x;
    int y;
};

void Point_Point(struct Point* this, int nx, int ny) {
{
        this->x = nx;
        this->y = ny;
        }
}

void Point_print(struct Point* this){
                printf("Point(%d, %d)\n", this->x, this->y);
        }
int Point_distance_squared(struct Point* this){
        return this->x * this->x + this->y * this->y;
        }
void Point_move(struct Point* this, int dx, int dy){
                this->x = this->x + dx;
                this->y = this->y + dy;
        }

int main_(){
        struct Point* temp_0 = runtime_alloc(sizeof(struct Point));
        Point_Point(temp_0, 3, 4);
        struct Point* p = temp_0;
        Point_print(p);
        int dist = Point_distance_squared(p);
                printf("Distance squared: %d\n", dist);
        Point_move(p, 1, 1);
        Point_print(p);
                return 0;
                }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
