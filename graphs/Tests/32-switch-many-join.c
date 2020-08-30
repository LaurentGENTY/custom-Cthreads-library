#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread.h"

#define NUMBER_OF_ITE 25
#define INTERVAL_THREAD 50
#define MAX_THREAD 1000
#define INTERVAL_YIELD 50
#define MAX_YIELD 1000
#define STAY_YIELD 50
#define STAY_THREAD 50

/* test de plein de switch pendant que N-1 threads sont bloqués dans join
 *
 * la durée du programme doit etre proportionnelle au nombre de threads et de yields donnés en argument
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static FILE* file1;
static FILE* file2;
static int nofor;
static unsigned long global_k;

static void * thfunc(void *_nbth)
{
  unsigned long nbth = (unsigned long) _nbth;
  unsigned long k,j;
  if ((unsigned long) nbth > 0) {
    thread_t th;
    int err;
    void *res;
    err = thread_create(&th, thfunc, _nbth-1);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);
    assert(res == _nbth-1);
  } else {
    int i;
    struct timeval tv1, tv2;
    unsigned long us=0;
    if(nofor){
        for(j=0;j<NUMBER_OF_ITE;j++){
            gettimeofday(&tv1, NULL);
            for(i=0; i<STAY_YIELD; i++)
                thread_yield();
            gettimeofday(&tv2, NULL);
            us = us + (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
        }
        us = us/NUMBER_OF_ITE;
        fprintf(file1,"%lu   %lu\n",global_k,us);
   }else{
       for(k=1;k<MAX_YIELD;k=k+INTERVAL_YIELD){
           us = 0;
           for(j=0;j<NUMBER_OF_ITE;j++){
               gettimeofday(&tv1, NULL);
               for(i=0; i<k; i++)
                    thread_yield();
                gettimeofday(&tv2, NULL);
                us = us + (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
            }
            us = us/NUMBER_OF_ITE;
            fprintf(file2,"%lu   %lu\n",k,us);
        }
    }
  }
  return _nbth;
}

int main(int argc, char *argv[])
{
  unsigned long k;

  char * str = malloc(strlen(argv[2])*sizeof(char)+strlen("_thread.dat")*sizeof(char));
  char * str1 = malloc(strlen(argv[2])*sizeof(char)+strlen("_yield.dat")*sizeof(char));
  strcpy(str,argv[2]);
  strcpy(str1,argv[2]);
  strcat(str,"_thread.dat");
  strcat(str1,"_yield.dat");
  file1 = fopen(str,"w");
  file2 = fopen(str1,"w");
  free(str);
  free(str1);
  nofor=0;
  thfunc((void*) STAY_THREAD);
  nofor=1;
  for(k=1;k<MAX_THREAD;k=k+INTERVAL_THREAD){
      global_k=k;
      thfunc((void*) k);
  }
  fclose(file1);
  fclose(file2);
  return 0;
}
