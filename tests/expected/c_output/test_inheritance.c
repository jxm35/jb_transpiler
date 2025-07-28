#include "runtime.h"
#include<stdio.h>





struct vtable {
    void (*speak)(void* this);
};

void Animal_speak(void* this_param);
void Dog_speak(void* this_param);
void Cat_speak(void* this_param);

static struct vtable Animal_vtable = {
    .speak = Animal_speak,
};

static struct vtable Dog_vtable = {
    .speak = Dog_speak,
};

static struct vtable Cat_vtable = {
    .speak = Cat_speak,
};

struct Animal {
    struct vtable* vtable;
    int age;
};

void Animal_Animal(struct Animal* this, int a) {
    this->vtable = &Animal_vtable;
{
        this->age = a;
        }
}

void Animal_speak(void* this_param) {
    struct Animal* this = (struct Animal*)this_param;
{
                printf("Animal sound\n");
        }
}

void Animal_sleep(struct Animal* this){
                printf("Sleeping for %d hours\n", this->age / 2);
        }
int Animal_getAge(struct Animal* this){
        return this->age;
        }

struct Dog {
    struct Animal parent;
    int breed_id;
};

void Dog_Dog(struct Dog* this, int a, int b) {
    Animal_Animal(&(this->parent), a);
    (this->parent).vtable = &Dog_vtable;
{
        this->breed_id = b;
        }
}

void Dog_speak(void* this_param) {
    struct Dog* this = (struct Dog*)this_param;
{
                printf("Woof! I'm %d years old\n", this->parent.age);
        }
}

void Dog_wagTail(struct Dog* this){
                printf("Wagging tail happily\n");
        }
int Dog_getBreed(struct Dog* this){
        return this->breed_id;
        }

struct Cat {
    struct Animal parent;
    int lives;
};

void Cat_Cat(struct Cat* this, int a) {
    Animal_Animal(&(this->parent), a);
    (this->parent).vtable = &Cat_vtable;
{
        this->lives = 9;
        }
}

void Cat_speak(void* this_param) {
    struct Cat* this = (struct Cat*)this_param;
{
                printf("Meow! I have %d lives left\n", this->lives);
        }
}

void Cat_purr(struct Cat* this){
                printf("Purring contentedly\n");
        }

int main_(){
                printf("=== Animal Inheritance Test ===\n");
        struct Dog* temp_0 = runtime_alloc(sizeof(struct Dog));
        Dog_Dog(temp_0, 3, 42);
        struct Dog* dog = temp_0;
        struct Cat* temp_1 = runtime_alloc(sizeof(struct Cat));
        Cat_Cat(temp_1, 2);
        struct Cat* cat = temp_1;
                printf("\n--- Dog Tests ---\n");
        dog->parent.vtable->speak(dog);
        Animal_sleep(&((dog)->parent));
        Dog_wagTail(dog);
                printf("Dog age: %d\n", Animal_getAge(&((dog)->parent)));
                printf("Dog breed: %d\n", Dog_getBreed(dog));
                printf("\n--- Cat Tests ---\n");
        cat->parent.vtable->speak(cat);
        Animal_sleep(&((cat)->parent));
        Cat_purr(cat);
                printf("Cat age: %d\n", Animal_getAge(&((cat)->parent)));
                printf("\n--- Parent Method Calls ---\n");
        struct Animal* temp_2 = runtime_alloc(sizeof(struct Animal));
        Animal_Animal(temp_2, 5);
        struct Animal* animal = temp_2;
        animal->vtable->speak(animal);
        Animal_sleep(animal);
                printf("Animal age: %d\n", Animal_getAge(animal));
                printf("\n--- Non virtual method call on upcast ---\n");
                animal = &((dog)->parent);
        animal->vtable->speak(animal);
                                return 0;
                                }

int main() {
    runtime_init();
    main_();
    runtime_shutdown();
}
