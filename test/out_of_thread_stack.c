#include <stdio.h>
#include <thread.h>
#include <assert.h>
#include <sys/time.h>
#include <string.h>

#define STACK_SIZEF8P1 (8540)
#define THREAD_NB 8

void recursif() {
    recursif();
}

void * out_of_stack(void * inte){
    thread_yield();
    // double tableau[STACK_SIZEF8P1] __attribute((unused));
    recursif();    

    return inte;
}

void* return_int(void* inte){
    thread_yield();
    return inte;
}

int main(int argc, char* argv[]){
    thread_t th[THREAD_NB];
    long i = 0;
    thread_create(&th[0],out_of_stack,(void*)0);
    for(i=1;i<THREAD_NB;i++){
        thread_create(&th[i],return_int,(void*)i);
    }
    void* ret;
    for(i=1;i<THREAD_NB;i++){
        thread_join(th[i],&ret);
        if((long)ret != i){
            printf("Error sigsegv affected the others threads\n");
            return 1;
        }
    }
    thread_join(th[0],&ret);
    if(ret!= NULL){
        printf("stack overflow undetected\n");
        return 1;
    }
    printf("Test successful\n");
    return 0;
}
