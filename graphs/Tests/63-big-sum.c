#include <stdio.h>
#include <thread.h>
#include <assert.h>
#include <sys/time.h>
#include <string.h>

#define NB_THREADS 1001
#define INTEGER 10000
#define INTERVAL_THREAD 100
#define MAX_INTEGER 100001
#define NUMBER_OF_ITE 5


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
    double us, result;
    long k;
    if(argc>3){
        FILE* file = fopen(argv[1],"a");
        unsigned long timeslice = atol(argv[2]);
        long i;
        void* res;
        int err = thread_mutex_init(&lock_result);
        assert(!err);
        err = thread_mutex_init(&lock_count);
        assert(!err);
        thread_t th[NB_THREADS];
        max = MAX_INTEGER;
        for(k=0;k<NUMBER_OF_ITE;k++){
            counter = 0;
            result=0;
            gettimeofday(&tv1,NULL);
            for (i = 0; i < NB_THREADS; i++){
                thread_create(&th[i], sum_array, (void*) NULL);
            }
            for (i = 0; i < NB_THREADS; i++){
                thread_join(th[i], &res);
                result = result + ((long)res);
            }
            gettimeofday(&tv2,NULL);
            us = us +(tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec) * 1e-3;
        }
        double true_val = (max*(max+1))/2;
        assert(true_val == result);
        us = us/NUMBER_OF_ITE;
        fprintf(file,"%lu   %lf\n",timeslice,us);
        fclose(file);
    }else{
        char * str = malloc(strlen(argv[2])*sizeof(char)+strlen("_integers.dat")*sizeof(char));
        char * str1 = malloc(strlen(argv[2])*sizeof(char)+strlen("_thread.dat")*sizeof(char));
        strcpy(str,argv[2]);
        strcpy(str1,argv[2]);
        strcat(str,"_integers.dat");
        strcat(str1,"_thread.dat");
        FILE* file1 = fopen(str,"w");
        FILE* file2 = fopen(str1,"w");
        free(str);
        free(str1);

        int err = thread_mutex_init(&lock_result);
        assert(!err);
        err = thread_mutex_init(&lock_count);
        assert(!err);



        thread_t* threads = malloc(sizeof(thread_t)*NB_THREADS);
        for(k=INTEGER;k<MAX_INTEGER;k+=INTEGER){
            max = k;
            long i;
            unsigned long l;
            void* res;
            us = 0;
            for(l=0;l<NUMBER_OF_ITE;l++){
                counter = 0;
                result=0;
                for (i = 0; i < NB_THREADS; i++){
                    thread_create(&threads[i], sum_array, (void*) NULL);
                }
                gettimeofday(&tv1,NULL);
                for (i = 0; i < NB_THREADS; i++){
                    thread_join(threads[i], &res);
                    result = result + ((long)res);
                }
                gettimeofday(&tv2,NULL);
                us = us +(tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec) * 1e-3;
            }
            double true_val = (max*(max+1))/2;
            assert(true_val == result);
            us = us/NUMBER_OF_ITE;
            fprintf(file1,"%lu   %lf\n",k,us);
        }
        free(threads);

        for(k=100;k<NB_THREADS;k+=INTERVAL_THREAD){
            thread_t* threads = malloc(sizeof(thread_t)*k);
            max = INTEGER;
            long i;
            unsigned long l;
            void* res;
            us = 0;
            for(l=0;l<NUMBER_OF_ITE;l++){
                counter = 0;
                result=0;
                for (i = 0; i < k; i++){
                    thread_create(&threads[i], sum_array, (void*) NULL);
                }
                gettimeofday(&tv1,NULL);
                for (i = 0; i < k; i++){
                    thread_join(threads[i], &res);
                    result = result + ((long)res);
                }
                gettimeofday(&tv2,NULL);
                us = us + (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec) * 1e-3;
            }
            double true_val = (max*(max+1))/2;
            assert(true_val == result);
            us = us/NUMBER_OF_ITE;
            fprintf(file2,"%lu   %lf\n",k,us);
            free(threads);
        }

        fclose(file1);
        fclose(file2);
        return 0;
    }
}
