#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>
#include "thread.h"

#define NUMBER_OF_ITE 25
#define INTERVAL_THREAD 10
#define MAX_THREAD 1000
#define INTERVAL_YIELD 10
#define MAX_YIELD 1000
#define STAY_YIELD 50
#define STAY_THREAD 50

/* test de plein de switch par plein de threads
 *
 * la durée du programme doit etre proportionnelle au nombre de threads et de yields donnés en argument
 *
 * support nécessaire:
 * - thread_create()
 * - thread_yield() depuis ou vers le main
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static void * thfunc(void *_nbyield)
{
  int nbyield = (intptr_t) _nbyield;
  int i;

  for(i=0; i<nbyield; i++)
    thread_yield();
  return NULL;
}

int main(int argc, char *argv[])
{
  int i, err;
  thread_t *ths;
  struct timeval tv1, tv2;
  unsigned long us,k,j;

  char * str = malloc(strlen(argv[2])*sizeof(char)+strlen("_thread.dat")*sizeof(char));
  char * str1 = malloc(strlen(argv[2])*sizeof(char)+strlen("_yield.dat")*sizeof(char));
  strcpy(str,argv[2]);
  strcpy(str1,argv[2]);
  strcat(str,"_thread.dat");
  strcat(str1,"_yield.dat");
  FILE* file1 = fopen(str,"w");
  FILE* file2 = fopen(str1,"w");
  free(str);
  free(str1);

  ths = malloc(MAX_THREAD * sizeof(thread_t));
  assert(ths);

  for(k=1;k<MAX_THREAD;k=k+INTERVAL_THREAD){
      us = 0;
      for(j=0;j<NUMBER_OF_ITE;j++){
          gettimeofday(&tv1, NULL);

          for(i=0; i<k; i++) {
              err = thread_create(&ths[i], thfunc, (void*) (intptr_t) STAY_YIELD);
              assert(!err);
          }

          for(i=0; i<STAY_YIELD; i++)
          thread_yield();

          for(i=0; i<k; i++) {
              void *res;
              err = thread_join(ths[i], &res);
              assert(!err);
              assert(res == NULL);
          }

          gettimeofday(&tv2, NULL);
          us = us + (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      }
      us = us/NUMBER_OF_ITE;
      fprintf(file1,"%lu   %lu\n",k,us);
  }

  for(k=1;k<MAX_YIELD;k=k+INTERVAL_YIELD){
      us = 0;
      for(j=0;j<NUMBER_OF_ITE;j++){
          gettimeofday(&tv1, NULL);

          for(i=0; i<STAY_THREAD; i++) {
              err = thread_create(&ths[i], thfunc, (void*) (intptr_t) k);
              assert(!err);
          }

          for(i=0; i<k; i++)
          thread_yield();

          for(i=0; i<STAY_THREAD; i++) {
              void *res;
              err = thread_join(ths[i], &res);
              assert(!err);
              assert(res == NULL);
          }

          gettimeofday(&tv2, NULL);
          us = us + (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
      }
      us = us/NUMBER_OF_ITE;
      fprintf(file2,"%lu   %lu\n",k,us);
  }

  fclose(file1);
  fclose(file2);
  free(ths);
  return 0;
}
