#include "thread.h"

thread_t thread_self(void)
{
    return current_thread;
}

int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg)
{
    set_preemptive_scheduler(0);
    if(newthread == NULL || func == NULL)
    return FAILURE;

    /* Créer le thread avec son identifiant unique */
    *newthread = add_new_thread_to_queue(generate_id(), func, funcarg);

    //TODO: check scheduling here
    //scheduler_load_next_thread();
    //thread_t old_thread = current_thread;
    //current_thread = next_thread;
    //swapcontext(&old_thread->context, &current_thread->context);

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_yield(void)
{
    set_preemptive_scheduler(0);
    scheduler_load_next_thread();
    if(next_thread->id == current_thread->id) {
        set_preemptive_scheduler(1);
        return SUCCESS;
    }
    else
        switch_to_scheduler_context();

    set_preemptive_scheduler(1);
    return SUCCESS;
}

int thread_join(thread_t thread, void **retval)
{
    set_preemptive_scheduler(0);

    /* Si le thread cible n'est pas encore terminé, se retirer de la runqueue et se placer temporairement dans la file de join lui étant associé */
    if(!thread->has_terminated) {
        CIRCLEQ_REMOVE(&runqueue, current_thread, runqueue_entries);
        STAILQ_INSERT_TAIL(&thread->joining_threads, current_thread, joining_queue_entries);
        thread_yield();
    }

    /* Si retval est non nul, lui assigner la valeur de retour du thread cible */
    if(retval != NULL)
    *retval = thread->retval;

    /* La structure thread cible peut être libérée */
    free_thread_memory(thread); //FIXME: free causing invalid writes

    set_preemptive_scheduler(1);
    return SUCCESS;
}

__attribute__((__noreturn__)) void thread_exit(void *retval)
{
    set_preemptive_scheduler(0);
    /* Enregistrer la valeur de retour du thread sortant et marquer ce thread comme terminé */
    current_thread->has_terminated = 1;
    current_thread->retval = retval;

    /* Repasser tous les threads en attente de terminaison sur ce thread dans la runqueue */
    thread_t i;
    //printf("Addresse de i dans thread exit : %p",&i);
    STAILQ_FOREACH(i, &current_thread->joining_threads, joining_queue_entries)
    CIRCLEQ_INSERT_BEFORE(&runqueue, current_thread, i, runqueue_entries);
    /* Retirer le thread de la runqueue */
    CIRCLEQ_REMOVE(&runqueue, current_thread, runqueue_entries);

    struct stack *stack = malloc(sizeof(struct stack));
    stack->sp = current_thread->context.uc_stack.ss_sp;
    STAILQ_INSERT_HEAD(&stackpool, stack, stackpool_entries);

    /* Si ce thread était le dernier, l'enregistrer dans la structure globale parent_context et lancer le contexte qui s'y trouve également */
    if(CIRCLEQ_EMPTY(&runqueue)) {
        pc.last_thread = current_thread;
        current_thread = NULL;
        setcontext(&pc.context);
    }

    /* Exécuter le thread suivant */
    scheduler_load_next_thread();
    current_thread = next_thread;
    set_preemptive_scheduler(1);
    setcontext(&current_thread->context);

    abort(); // Ne doit jamais être atteint
}
