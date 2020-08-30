#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include "thread.h"

#define LEN_TABLE 10000000
#define NB_THREAD 20
#define NUMBER_OF_ITE 20
#define INTERVAL (LEN_TABLE/NB_THREAD)

long to_search[LEN_TABLE];
long nb_a_trouver;
char a_trouve = 0;

static void * thfunc(void *nb)
{
  int index = (intptr_t) nb;
  int i;
  int k;
  for(i=0; i<INTERVAL; i++){
    for(k=0;k<100;k++){
        nb_a_trouver = nb_a_trouver;
        //printf("%d et  %d\n",index * INTERVAL + i,current_thread->id);
    }
    if(a_trouve){
        thread_exit((void*)2);
    }
    if(to_search[index* INTERVAL + i] == nb_a_trouver){
        a_trouve = 1;
        thread_exit((void*)1);
    }
  }
  return (void*)0;
}

int main(int argc, char *argv[])
{
    struct timeval tv1,tv2;
    unsigned long s=0;
    int k;
    long timeslice = atol(argv[2]);
    FILE* file = fopen(argv[1],"a");
    nb_a_trouver = (long) LEN_TABLE + 1;
    int seed = 42;
    srand(seed);
    long i =0;
    for(i=0; i<LEN_TABLE;i++){
        to_search[i] = 4;
    }
    to_search[(int)(9 * INTERVAL + INTERVAL/4)] = (long)LEN_TABLE +1;
    for(k=0;k<NUMBER_OF_ITE;k++){
        a_trouve =0;
        thread_t th[NB_THREAD];
        gettimeofday(&tv1, NULL);
        for(i=0;i<NB_THREAD;i++){
            thread_create(&th[i],thfunc,(void*)(i));
        }
        void* retval[NB_THREAD];
        for(i=0;i<NB_THREAD;i++){
            thread_join(th[i],&retval[i]);
        }
        gettimeofday(&tv2, NULL);
        s += (tv2.tv_sec - tv1.tv_sec)*1000000 + (tv2.tv_usec - tv1.tv_usec);
    }
    s = s/NUMBER_OF_ITE;
    fprintf(file,"%lu   %lu\n",timeslice,s);
    fclose(file);
    return 0;
}
