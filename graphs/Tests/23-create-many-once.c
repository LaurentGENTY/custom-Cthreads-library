#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include "thread.h"

#define NUMBER_OF_ITE 25
#define INTERVAL 10
#define MAX_PARAM 1000

/* test de plein de create, puis plein de join quand ils ont tous fini
 *
 * valgrind doit etre content.
 * la durée du programme doit etre proportionnelle au nombre de threads donnés en argument.
 * jusqu'à combien de threads cela fonctionne-t-il ?
 *
 * support nécessaire:
 * - thread_create()
 * - thread_exit()
 * - thread_join() sans récupération de la valeur de retour
 */

static void * thfunc(void *dummy __attribute__((unused)))
{
  thread_exit(NULL);
  return (void*) 0xdeadbeef; /* unreachable, shut up the compiler */
}

int main(int argc, char *argv[])
{
  thread_t *th;
  int err, i;
  struct timeval tv1, tv2;
  unsigned long us,k;

  if (argc < 2) {
    printf("argument manquant: nombre de threads\n");
    return -1;
  }


  th = malloc(MAX_PARAM*sizeof(*th));
  if (!th) {
    perror("malloc");
    return -1;
  }
  int j =0;
  FILE* file = fopen(argv[1],"w");
  for(k=1;k<MAX_PARAM;k=k+INTERVAL){
    us=0;
    for(j=0;j<NUMBER_OF_ITE;j++){
        gettimeofday(&tv1, NULL);

        /* on cree tous les threads */
        for(i=0; i<k; i++) {
            err = thread_create(&th[i], thfunc, NULL);
            assert(!err);
        }

        /* on leur passe la main, ils vont tous terminer */
        for(i=0; i<k; i++) {
            thread_yield();
        }

        /* on les joine tous, maintenant qu'ils sont tous morts */
        for(i=0; i<k; i++) {
            err = thread_join(th[i], NULL);
            assert(!err);
        }

        gettimeofday(&tv2, NULL);
        us = us +(tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
    }
    us = us/NUMBER_OF_ITE;
    fprintf(file,"%lu   %lu\n",k,us);
  }

  free(th);
  fclose(file);
  return 0;
}
