#include "thread.h"

unsigned stackpool_count;
struct stack_pool stackpool;
struct run_queue runqueue;
thread_t current_thread;
thread_t next_thread;
struct parent_context pc;
ucontext_t scheduler_context;
sigset_t sigprof_blocking_set;
struct itimerval scheduling_timer;

/* Nécessaires à l'exécution du main thread */
extern int main(int, char**);
static int Argc;
static char **Argv;

#if USE_PREEMPTIVE == 1
    inline void set_preemptive_scheduler(int on)
    {
        if(on) {
            sigprocmask(SIG_UNBLOCK, &sigprof_blocking_set, NULL);
            scheduling_timer.it_value.tv_usec = TIMESLICE;
        }
        else {
            sigprocmask(SIG_BLOCK, &sigprof_blocking_set, NULL);
            scheduling_timer.it_value.tv_usec = 0;
        }
        setitimer(ITIMER_PROF, &scheduling_timer, NULL);
    }
#else
    inline void set_preemptive_scheduler(int on) {}
#endif

void __debug_print_runqueue_state()
{
    printf("HEAD -> ");
    thread_t i;
    CIRCLEQ_FOREACH(i, &runqueue, runqueue_entries)  {
        printf("%d -> ", i->id);
    }
    printf("END\n");
    if(current_thread)
    printf("current_thread points to %d\n\n", current_thread->id);
}

void handle_error(const char *error)
{
    perror(error);
    exit(1);
}

inline void free_thread_memory(thread_t thread)
{
    VALGRIND_STACK_DEREGISTER(thread->valgrind_stackid);
    free(thread);
}

void run_thread(void *(*func)(void *, void*), void *funcarg)
{
    void *retval;

    if(func == (void*(*)(void*, void*))main) { // Le thread main appelle main
        main(Argc, Argv);
        retval = NULL;
    }
    else // Tous les threads autres que main appellent leurs routines respectives
    retval = (*func)(funcarg, NULL);
    thread_exit(retval);
}

unsigned int generate_id(void)
{
    static unsigned int id = 1;
    return id++;
}

thread_t add_new_thread_to_queue(int id, void *(*func)(void *), void *funcarg)
{
    // if(id == 50)
        // exit(1);
    /* Initialise le PCB du thread */
    thread_t new_thread = malloc(sizeof(*new_thread));
    new_thread->id = id;
    new_thread->retval = NULL;
    new_thread->has_terminated = 0;

    /* Initialise le contexte du thread */
    ucontext_t *context = &new_thread->context;
    getcontext(context);
    context->uc_link = NULL;
    sigemptyset(&context->uc_sigmask); // Le thread accepte tous les signaux

    /* Alloue la pile du thread et gère les éventuels débordements de pile */
    context->uc_stack.ss_size = STACK_SIZE + PAGE_SIZE;
    //if(posix_memalign(&context->uc_stack.ss_sp, PAGE_SIZE, context->uc_stack.ss_size) == -1)
    //    handle_error("posix_memalign failed");
    if(!STAILQ_EMPTY(&stackpool)) { 
        // printf("Thread %d reused stack\n", id);
        struct stack *stack = STAILQ_FIRST(&stackpool);
        context->uc_stack.ss_sp = stack->sp;
        STAILQ_REMOVE_HEAD(&stackpool, stackpool_entries);
        free(stack);
    }
    else {
        // printf("Thread %d allocated new stack\n", id);
        context->uc_stack.ss_sp = mmap(NULL, context->uc_stack.ss_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if(mprotect(context->uc_stack.ss_sp, PAGE_SIZE, PROT_NONE) == -1)
            handle_error("mprotect failed");
    }

    new_thread->valgrind_stackid = VALGRIND_STACK_REGISTER(context->uc_stack.ss_sp, context->uc_stack.ss_sp + context->uc_stack.ss_size);

    makecontext(context, (void (*)(void))run_thread, 2, func, funcarg);
    STAILQ_INIT(&new_thread->joining_threads);

    /* Le thread est enfilé dans la runqueue */
    if(CIRCLEQ_EMPTY(&runqueue))
        CIRCLEQ_INSERT_HEAD(&runqueue, new_thread, runqueue_entries); //TODO: Play with multiple scheduling policies
    else
        CIRCLEQ_INSERT_AFTER(&runqueue, current_thread, new_thread, runqueue_entries);

    // __debug_print_runqueue_state();

    return new_thread;
}

void stack_handler()
{
    thread_exit(NULL);
}

void master_context(int argc, char **argv)
{
    next_thread = NULL;

    // argc et argv sont stockés globalement (il y a sûrement mieux à faire, mais ça reste pratique dans un premier temps)
    Argc = argc;
    Argv = argv;

    /* Initialise la runqueue */
    CIRCLEQ_INIT(&runqueue);

    /* Initialise la file des piles disponibles */
    STAILQ_INIT(&stackpool);
    stackpool_count = 0;

    /* sigprof_blocking_set est un set de signaux dont seul le signal SIGPROF est activé */
    sigemptyset(&sigprof_blocking_set);
    sigaddset(&sigprof_blocking_set, SIGPROF);

    /* Programme le timer pour qu'il se déclenche toutes les TIMESLICE millisecondes. Il n'est pas encore lancé. */
    scheduling_timer.it_interval.tv_sec = 0;
    scheduling_timer.it_interval.tv_usec = 0;
    scheduling_timer.it_value.tv_sec = 0;
    scheduling_timer.it_value.tv_usec = 0;

    /* Associe à chaque déclenchement de timer l'exécution de la routine scheduler_handler */
    sigset_t scheduler_mask;
    sigfillset(&scheduler_mask);
    sigdelset(&scheduler_mask, SIGINT);
    struct sigaction scheduler_sigaction;
    scheduler_sigaction.sa_sigaction = scheduler_handler;
    scheduler_sigaction.sa_mask = scheduler_mask;
    scheduler_sigaction.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &scheduler_sigaction, NULL);

    /* Alloue les piles de l'ordonnanceur */
    getcontext(&scheduler_context);
    scheduler_context.uc_stack.ss_size = STACK_SIZE;
    scheduler_context.uc_stack.ss_sp = malloc(STACK_SIZE);
    int scheduler_stack_stackid = VALGRIND_STACK_REGISTER(scheduler_context.uc_stack.ss_sp, scheduler_context.uc_stack.ss_sp + scheduler_context.uc_stack.ss_size);
    scheduler_context.uc_stack.ss_flags = 0;
    sigfillset(&scheduler_context.uc_sigmask);
    sigdelset(&scheduler_context.uc_sigmask, SIGINT);
    makecontext(&scheduler_context, scheduler_dispatch, 0);

    /* Initialise le gestionnaire de débordement de pile */
    struct sigaction sigsegv_sigaction;
    sigsegv_sigaction.sa_sigaction = stack_handler;
    stack_t ss;
    ss.ss_size = SIGSTKSZ;
    ss.ss_flags = 0;
    ss.ss_sp = mmap(NULL, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if(ss.ss_sp == MAP_FAILED)
        handle_error("Error while mmap\n");
    if(sigaltstack(&ss, NULL)==-1)
        handle_error("Error while allocating sigaltstack \n");
    sigemptyset(&sigsegv_sigaction.sa_mask);
    sigsegv_sigaction.sa_flags = SA_ONSTACK;
    if(sigaction(SIGSEGV, &sigsegv_sigaction, NULL)==-1)
        handle_error("Can't load a sigsegv handler\n");

    /* Bloque les signaux dans le contexte principal */
    sigfillset(&pc.context.uc_sigmask);
    sigdelset(&pc.context.uc_sigmask, SIGINT);
    set_preemptive_scheduler(0); 

    /* Créer le main thread avec l'identifiant 0 */
    current_thread = add_new_thread_to_queue(0, (void *(*)(void*))main, NULL); // Les arguments du main passeront par les variables globales Argc et Argv

    /* Le main thread s'exécute : nous ne reviendrons ici qu'à la mort du dernier thread */
    swapcontext(&pc.context, &current_thread->context);

    /* Fin du programme : libération des ressources du dernier thread, des gestionnaires et de la stack_pool */
    free_thread_memory(pc.last_thread);
    VALGRIND_STACK_DEREGISTER(scheduler_stack_stackid);
    free(scheduler_context.uc_stack.ss_sp);
    munmap(ss.ss_sp, SIGSTKSZ);

    while(!STAILQ_EMPTY(&stackpool)) {
        struct stack *s = STAILQ_FIRST(&stackpool);
        STAILQ_REMOVE_HEAD(&stackpool, stackpool_entries);
        munmap(s->sp, STACK_SIZE + PAGE_SIZE);
        free(s);
    }

    exit(0); // Fin idéale du programme
}

void scheduler_load_next_thread()
{
    next_thread = CIRCLEQ_LOOP_NEXT(&runqueue, current_thread, runqueue_entries);
}

void scheduler_dispatch()
{
    current_thread = next_thread;
    set_preemptive_scheduler(1);
    setcontext(&current_thread->context);
}

void switch_to_scheduler_context()
{
    swapcontext(&current_thread->context, &scheduler_context);
}

void scheduler_handler(int signum, siginfo_t *nfo, void *context)
{
    scheduler_load_next_thread();
    if(next_thread->id == current_thread->id)
        return;
    else
        switch_to_scheduler_context();
}

