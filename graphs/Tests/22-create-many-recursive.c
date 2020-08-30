#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread.h"


#define NUMBER_OF_ITE 25
#define INTERVAL 10
#define MAX_PARAM 1000

/* test de plein de create-destroy récursif.
 *
 * valgrind doit etre content.
 * la durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 * jusqu'à combien de threads cela fonctionne-t-il ?
 *
 * support nécessaire:
 * - thread_create()
 * - retour sans thread_exit()
 * - thread_join() avec récupération de la valeur de retour
 */

static void * thfunc(void *_nb)
{
  unsigned long nb = (unsigned long) _nb;
  if ((unsigned long) nb > 0) {
    thread_t th;
    int err;
    void *res;
    err = thread_create(&th, thfunc, _nb-1);
    assert(!err);
    err = thread_join(th, &res);
    assert(!err);
    assert(res == _nb-1);
  }
  return _nb;
}

int main(int argc, char *argv[])
{
  unsigned long k,j;
  struct timeval tv1, tv2;
  unsigned long us;

  if (argc < 2) {
    printf("argument manquant: nombre de threads\n");
    return -1;
  }

  FILE* file = fopen(argv[1],"w");
  for(k=1;k<MAX_PARAM;k=k+INTERVAL){
    us=0;
    for(j=0;j<NUMBER_OF_ITE;j++){
        gettimeofday(&tv1, NULL);
        thfunc((void*) k);
        gettimeofday(&tv2, NULL);
        us =us + (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
    }
    us = us/NUMBER_OF_ITE;
    fprintf(file,"%lu   %lu\n",k,us);
  }
  fclose(file);
  return 0;
}
