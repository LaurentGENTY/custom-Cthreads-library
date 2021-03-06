#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include "thread.h"

#define INTERVAL 1000
#define MAX_LEN 10001
#define NUMBER_OF_ITE 1
long* tab;

void* merge(long start,long middle,long end,long* tab1,long* tab2){
    long* table = malloc(sizeof(long)*(end-start));
    long i=0;
    long j=0;
    long leftpoint = 0;
    long rightpoint = 0;
    long end1 = middle-start;
    long end2 = end-middle;
    long total_len = end-start;
    for(i=0;i<total_len;i++){
        if(leftpoint>=end1){
            for(j=0;j<(end2-rightpoint);j++){
                table[i+j] = tab2[rightpoint+j];
            }
            break;
        }else if(rightpoint>=end2){
            for(j=0;j<(end1-leftpoint);j++){
                table[i+j] = tab1[leftpoint+j];
            }
            break;
        }else{
            if(tab1[leftpoint]>tab2[rightpoint]){
                table[i] = tab2[rightpoint];
                rightpoint++;
            }else{
                table[i] = tab1[leftpoint];
                leftpoint++;
            }
        }
    }
    return (void*) table;
}

void* merge_sort(void* arg){
    int err;
    thread_t th;
    thread_t th2;
    long* args = (long*) arg;
    long start = args[0];
    long end = args[1];
    void* tab1=NULL;
    void* tab2=NULL;
    if(end-1>start){
        int middle = (start+end)/2;
        long compute[2] = {start,middle};
        long compute2[2] = {middle,end};
        thread_yield();
        err = thread_create(&th, merge_sort, (void*)compute);
        assert(!err);
        thread_yield();
        err = thread_create(&th2, merge_sort, (void*)compute2);
        assert(!err);
        err = thread_join(th, &tab1);
        assert(!err);
        err = thread_join(th2, &tab2);
        assert(!err);
        void* sorted_tab = merge(start,middle,end,(long*)tab1,(long*)tab2);
        free(tab1);
        free(tab2);
        return sorted_tab;
    }else{
        long* tableau = malloc(sizeof(long));
        tableau[0] = (((long*) tab)[start]);
        return (void*) tableau;
    }
}

int main(int argc, char* argv[]){
    struct timeval tv1,tv2;
    unsigned long len = 0;
    int j=0;
    int seed = 42;
    srand(seed);

    FILE* file = fopen(argv[1],"w");

    for(len=1000;len<MAX_LEN;len+=INTERVAL){
        unsigned long us=0;
        tab = (long *) malloc(len*sizeof(long));
        int i =0;
        for(i=0; i<len;i++){
            tab[i] = rand()%(10*len);
        }
        for(j=0;j<NUMBER_OF_ITE;j++){
            long zero = 0;
            long compute[2] = {zero,len};
            gettimeofday(&tv1, NULL);
            void* table = merge_sort((void*) compute);
            gettimeofday(&tv2, NULL);
            us = us + (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec) * 1e-3;
            free(table);
        }
        us = us/NUMBER_OF_ITE;
        fprintf(file,"%lu   %lu\n",len,us);
        free(tab);
    }
    fclose(file);
    return 0;
}
