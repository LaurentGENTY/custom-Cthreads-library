#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread.h"

#define NUMBER_OF_ITE 5
#define INTERVAL 3
#define MAX_PARAM 21

/* fibonacci.
 *
 * la durée doit être proportionnel à la valeur du résultat.
 * valgrind doit être content.
 * jusqu'à quelle valeur cela fonctionne-t-il ?
 *
 * support nécessaire:
 * - thread_create()
 * - thread_join() avec récupération de la valeur de retour
 * - retour sans thread_exit()
 */

static void * fibo(void *_value)
{
  thread_t th, th2;
  int err;
  void *res = NULL, *res2 = NULL;
  unsigned long value = (unsigned long) _value;
  /* on passe un peu la main aux autres pour eviter de faire uniquement la partie gauche de l'arbre */
  thread_yield();

  if (value < 3)
    return (void*) 1;

  err = thread_create(&th, fibo, (void*)(value-1));
  assert(!err);
  err = thread_create(&th2, fibo, (void*)(value-2));
  assert(!err);

  err = thread_join(th, &res);
  assert(!err);
  err = thread_join(th2, &res2);
  assert(!err);

  return (void*)((unsigned long) res + (unsigned long) res2);
}

int main(int argc, char *argv[])
{
  unsigned long k;
  struct timeval tv1, tv2;
  double us;
  int j;

  FILE* file = fopen(argv[1],"w");
  for(k=1;k<MAX_PARAM;k=k+INTERVAL){
    us=0;
    for(j=0;j<NUMBER_OF_ITE;j++){
        gettimeofday(&tv1, NULL);
        (unsigned long) fibo((void *)k);
        gettimeofday(&tv2, NULL);
        us = us +(tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec) * 1e-6;
    }
    us = us/NUMBER_OF_ITE;
    fprintf(file,"%lu   %lf\n",k,us);
  }

  return 0;
}
