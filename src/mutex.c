#include "thread.h"

int thread_mutex_init(thread_mutex_t *mutex)
{
    set_preemptive_scheduler(0);
    if(mutex == NULL)
        return FAILURE;

    mutex->status = UNLOCKED;
    STAILQ_INIT(&mutex->waiting_threads);

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_mutex_destroy(thread_mutex_t *mutex)
{
    set_preemptive_scheduler(0);
    if(mutex == NULL)
        return FAILURE;

    mutex->status=UNLOCKED;
    //TODO: dequeue all pending threads

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_mutex_try_lock(thread_mutex_t *mutex)
{
    set_preemptive_scheduler(0);
    if (mutex == NULL)
        return FAILURE;

    if (mutex->status == LOCKED)
         return FAILURE;

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_mutex_lock(thread_mutex_t *mutex) //TODO: prevent signals
{
    set_preemptive_scheduler(0);
    if(mutex == NULL)
        return FAILURE;

    while(mutex->status == LOCKED) { 
        CIRCLEQ_REMOVE(&runqueue, current_thread, runqueue_entries);
        STAILQ_INSERT_TAIL(&mutex->waiting_threads, current_thread, mutex_queue_entries);
        thread_yield();
    }

    mutex->status = LOCKED;

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_mutex_unlock(thread_mutex_t *mutex)
{
    set_preemptive_scheduler(0);
    if(mutex == NULL)
        return FAILURE;

    mutex->status = UNLOCKED;
    
    if(!STAILQ_EMPTY(&mutex->waiting_threads)) {
        thread_t thread_to_get_back = STAILQ_FIRST(&mutex->waiting_threads);
        STAILQ_REMOVE_HEAD(&mutex->waiting_threads, mutex_queue_entries);
        CIRCLEQ_INSERT_AFTER(&runqueue, current_thread, thread_to_get_back, runqueue_entries); //TODO: think about insertion position
    }
    thread_yield();

    set_preemptive_scheduler(1);
    return SUCCESS;
}
