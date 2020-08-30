#include <ucontext.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <valgrind/valgrind.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>

#ifndef __THREAD_H__
#define __THREAD_H__

#ifndef USE_PTHREAD

#define SUCCESS 0
#define FAILURE -1
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define STACK_SIZE (16 * PAGE_SIZE)
#define TIMESLICE 15000 // In microseconds
#define USE_PREEMPTIVE 0

/* Structure du PCB d'un thread */
struct thread {
    unsigned int id;                                // Identifiant unique permettant de reconnaître un thread
    ucontext_t context;                             // Contexte contenant les registres, le masque de signal et la pile courante
    void *retval;                                   // Valeur de retour du thread
    CIRCLEQ_ENTRY(thread) runqueue_entries;         // Nécessaire à la gestion de la run_queue
    STAILQ_ENTRY(thread) mutex_queue_entries;       // Nécessaire à la gestion de la waiting_queue
    STAILQ_HEAD(, thread) joining_threads;          // File des threads en attente de terminaison de ce thread
    STAILQ_ENTRY(thread) joining_queue_entries;     // Nécessaire à la gestion de la file des joining_threads
    int has_terminated;                             // Booléen qui détermine si le thread a terminé son exécution
    int valgrind_stackid;                           // Permet à valgrind de localiser la pile du thread
};

/* Structure représentant une pile disponible et en attente d'être assignée à un thread */
struct stack {
    void *sp;                                       // Pointeur vers la pile
    STAILQ_ENTRY(stack) stackpool_entries;          // Nécessaire à la gestion de la stack_pool
};

/* Structure représentant un mutex */
enum lock_status {LOCKED, UNLOCKED};
typedef struct thread_mutex {
    enum lock_status status;                        // Différents états possible d'un mutex
    STAILQ_HEAD(, thread) waiting_threads;          // File de threads en attente du lock de ce mutex
} thread_mutex_t;

/* Structure de gestion de la runqueue */
CIRCLEQ_HEAD(run_queue, thread);
typedef struct thread *thread_t;                    // Identifiant de thread, pointeur sur la structure PCB

STAILQ_HEAD(stack_pool, stack);

/* Structure permettant de contenir le contexte principal, nécessaire pour libérer les ressources à la mort du dernier thread */
struct parent_context {
    thread_t last_thread;                           // Dernier thread terminé dont il faut libérer les ressources
    ucontext_t context;                             // Contexte du gestionnaire
};

extern struct run_queue runqueue;                   // File des tâches prêtes à être exécutées
extern struct stack_pool stackpool;                 // File des piles disponibles
extern unsigned stackpool_count;                    // Nombre de piles disponibles
extern thread_t current_thread;                     // Pointeur vers le thread en cours d'exécution
extern thread_t next_thread;                        // Pointeur vers le prochain thread choisi par l'ordonnanceur
extern struct parent_context pc;                    // Contexte principal fonctionnant sur la "vraie" pile du programme
extern ucontext_t scheduler_context;                // Contexte de l'ordonnanceur, sur lequel on switche à chaque yield et chaque dispatch
extern sigset_t sigprof_blocking_set;               // Set dont seul le signal SIGPROF sera actif, nécessaire à l'activation et la désactivation de la préemption
extern struct itimerval scheduling_timer;           // Timer utilisé par le scheduler

/* Fonctions de l'API devant impérativement être implémentées */
extern thread_t thread_self(void);
extern int thread_create(thread_t *newthread, void *(*func)(void *), void *funcarg);
extern int thread_yield(void);
extern int thread_join(thread_t thread, void **retval);
extern void thread_exit(void *retval) __attribute__ ((__noreturn__));

extern int thread_mutex_init(thread_mutex_t *mutex);
extern int thread_mutex_destroy(thread_mutex_t *mutex);
extern int thread_mutex_lock(thread_mutex_t *mutex);
extern int thread_mutex_unlock(thread_mutex_t *mutex);
extern int thread_mutex_try_lock(thread_mutex_t *mutex);




/* C'est dans cette fonction que les différentes politiques d'ordonnancement pourront être testées */
extern void scheduler_load_next_thread();

/* Fonction exécutée par le scheduler qui passe la main au thread suivant dans la runqueue */
extern void scheduler_dispatch();

/* Fonctions utilitaires */ //TODO: A déplacer dans un utils.h avec les fonctions de scheduler
extern void free_thread_memory(thread_t thread);

/*
* Handler exécuté par tous les threads. Le handler exécute la fonction préalablement associée au thread via thread_create,
* puis appelle thread_exit si le thread ne le fait pas explicitement à l'issue de son exécution. */
extern void run_thread(void *(*func)(void *, void*), void *funcarg);

/* Génère un identifiant de thread unique */
extern unsigned int generate_id(void);

/* Ajouter un nouveau thread dans la runqueue après lui avoir assigné la routine func munie de son argument funcarg */
extern thread_t add_new_thread_to_queue(int id, void *(*func)(void *), void *funcarg);

/* Initialiser la bibliothèque de threads : intialise la runqueue et alloue un thread correspondant au fil d'exécution du main (pour que la fonction s'exécute au début du programme) */
extern void master_context(int argc, char **argv) __attribute__ ((constructor));

/* Affiche error dans la sortie d'erreur et termine le programme */
extern void handle_error(const char *error);

/* Bascule vers le contexte de l'ordonnanceur */
extern void switch_to_scheduler_context();

/* Routine exécutée à chaque SIGPROF, si la préemption est activée */
extern void scheduler_handler(int signum, siginfo_t *nfo, void *context);

/* Active ou desactive la préemption */
extern void set_preemptive_scheduler(int on);

#else /* USE_PTHREAD */

/* Si on compile avec -DUSE_PTHREAD, ce sont les pthreads qui sont utilisés */
#include <sched.h>
#include <pthread.h>
#define thread_t pthread_t
#define thread_self pthread_self
#define thread_create(th, func, arg) pthread_create(th, NULL, func, arg)
#define thread_yield sched_yield
#define thread_join pthread_join
#define thread_exit pthread_exit

/* Interface possible pour les mutex */
#define thread_mutex_t            pthread_mutex_t
#define thread_mutex_init(_mutex) pthread_mutex_init(_mutex, NULL)
#define thread_mutex_destroy      pthread_mutex_destroy
#define thread_mutex_lock         pthread_mutex_lock
#define thread_mutex_unlock       pthread_mutex_unlock

#endif /* USE_PTHREAD */

#endif /* __THREAD_H__ */
