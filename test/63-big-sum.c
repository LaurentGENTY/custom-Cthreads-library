#include <stdio.h>
#include <thread.h>
#include <assert.h>
#include <sys/time.h>

#define NB_THREADS 1000


long counter;
long max;
thread_mutex_t lock_result;
thread_mutex_t lock_count;


void* sum_array(void* arg)
{
    long i = 0;
    long result = 0;
    thread_mutex_lock(&lock_count);
    i = counter;
    counter++;
    thread_mutex_unlock(&lock_count);
    thread_yield();
    while(i<max) {
        thread_mutex_lock(&lock_result);
        result += i+1;
        thread_mutex_unlock(&lock_result);
        thread_yield();
        thread_mutex_lock(&lock_count);
        i = counter;
        counter++;
        thread_mutex_unlock(&lock_count);
        thread_yield();
    }
    return (void*) result;
}

int main(int argc, char*argv[])
{
    struct timeval tv1, tv2;
    if(argc<2){
        printf("Il manque la taille du tableau");
        return -1;
    }
    max = atol(argv[1]);
    int nb_thread = NB_THREADS;
    if(argc>3){
        nb_thread = atoi(argv[2]);
    }
    int err = thread_mutex_init(&lock_result);
    assert(!err);
    err = thread_mutex_init(&lock_count);
    assert(!err);
    long i;
    counter = 0;
    thread_t* threads = malloc(sizeof(thread_t)*nb_thread);
    double result=0;
    void* res;
    for (i = 0; i < nb_thread; i++)
        thread_create(&threads[i], sum_array, (void*) NULL);
    gettimeofday(&tv1,NULL);
    for (i = 0; i < nb_thread; i++){
        thread_join(threads[i], &res);
        result = result + ((long)res);
    }
    gettimeofday(&tv2,NULL);
    double true_val = (max*(max+1))/2;

    assert(true_val == result);
    double s = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) * 1e-6;
    printf("Le calcul est juste et la somme des %ld premiers entiers vaut : %e\n",max,result);
    printf("Le calcul à pris %e secondes\n",s);
    // assert(sum == (NUMBER*(NUMBER+1))/2);
    free(threads);
    return 0;
}
