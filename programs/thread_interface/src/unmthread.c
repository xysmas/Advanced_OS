/* Advanced Operating Systems
 * Fall 2015,
 * Programming assignment 1
 * Aaron Gonzales
 */
#include "unmthread.h"


int INIT = 0;
int THREAD_RUNNING = 0;
int THREAD_COUNTER = 0;
int AM_WAITING = 0;

/* main context initialized statically */
static ucontext_t main_context;

/* initializes the queue and structures... */
void initialize()
{
   debug("initializing thread library");
   queue = g_queue_new();
   getcontext(&main_context);
   INIT = 1;
}


void thread_runner(thrfunc_t f)
{
  CURRENT_THREAD->status = RUNNING;
  CURRENT_THREAD->ret_val = f(CURRENT_THREAD->f_args);
  unmthread_exit(CURRENT_THREAD->ret_val);
}


/* Create a thread starting at the running function with the supplied argument and scheduling 
 * information. In non-preemptive code, the created thread will not actually run until the
 * current thread yields.  */
int unmthread_create(unmthread_t *thr, thrfunc_t f, void *arg, void *schedinfo)
{
  if (!INIT) initialize();
  /* debug("malloc thread"); */
  thr = (unmthread_t*) malloc(sizeof(unmthread_t));
  thr->id = THREAD_COUNTER;
  THREAD_COUNTER++;
  thr->status = READY;
  thr->ret_val = NULL;
  thr->f_args = arg;
  thr->schedule_info = schedinfo;
  thr->join_data = NULL;
  getcontext(&thr->context);

  thr->context.uc_stack.ss_sp = thr->stack = malloc(THREAD_STACK_SIZE);
  assert(thr->stack != 0); /* fail if we failed to allocate stack */

  thr->context.uc_link = 0; /* we are manually going to control the execution */
  thr->context.uc_stack.ss_size = THREAD_STACK_SIZE;

  makecontext(&thr->context, (void (*)(void))thread_runner, 1, f);
  /* g_queue_push_tail(queue, thr); */
  g_queue_push_tail(queue, thr);
  /* CURRENT_THREAD = thr; */
  getcontext(&main_context);

  /* getcontext(&main_context); */
  /* unmthread_yield(); */
  return 0;
}


/* Cause the current thread to yield execution to another thread if one is availble to 
 * run on this processor */
int unmthread_yield(void){
  if (!CURRENT_THREAD){
    CURRENT_THREAD = g_queue_pop_head(queue);
    if(!CURRENT_THREAD) return -1;
    swapcontext(&main_context, &CURRENT_THREAD->context);
  }
  debug("Beginning yield; Current thread: %i", CURRENT_THREAD->id);

  if (g_queue_get_length(queue) > 0){
    NEXT_THREAD = g_queue_pop_head(queue);
  }

  if (CURRENT_THREAD != NEXT_THREAD){
    debug("swapping threads; thread %i", CURRENT_THREAD->id);
    TMP_THREAD = CURRENT_THREAD;
    CURRENT_THREAD = NEXT_THREAD;
    g_queue_push_tail(queue, TMP_THREAD);
    swapcontext(&TMP_THREAD->context, &NEXT_THREAD->context);
  }

  return 0;
}

/* Wait for termination of the supplied thread, returning its provided return value
 * to the caller in the retval argument */
int unmthread_join(unmthread_t *thr, void **retval){
  if(!thr){ 
    printf("Thread is uninitialized");
    return -1;
  }
  /* this is a bad hack to even get it moving at all */
  if (thr->status != DONE) unmthread_yield(); 
  thr->join_data = *retval;
  return 0;
}

/* Terminate execution of this thread, returning the provided argument to a thread that 
 * joins with this thread */
int unmthread_exit(void *retval){
  CURRENT_THREAD->status = DONE;
  if (g_queue_get_length(queue) > 0){
  }
  unmthread_t *next =  g_queue_pop_head(queue);
  if (next == NULL) return 0;
  next->ret_val = retval;
  debug("exiting exit ");
  unmthread_yield();
  return 0;
}

/* Return a pointer to the currently running thread. Frequently used to look up thread-specific
 * information in a hash table */
unmthread_t *unmthread_current(void){
  /* wrong; need to handle differently */
  assert(CURRENT_THREAD);
  /* debug("returning current thread"); */
  return CURRENT_THREAD;
}

int wait_for_threads()
{
  AM_WAITING = 1;
  debug("Waiting for threads");
  while (g_queue_get_length(queue) >= 1){
    unmthread_yield();
    debug("Waiting for threads...");
  }
  debug("exiting Waiting for threads...");
  AM_WAITING = 0;
  return 0;
}



int unmthread_cond_init(unmcond_t *c);
int unmthread_cond_wait(unmcond_t *c, unmmutex_t *m);
int unmthread_cond_signal(unmcond_t *c);
int unmthread_cond_uninit(unmcond_t *c);

/* initalizes the counter to zero. safe for single threads */
int unmthread_mutex_init(unmmutex_t *m){
  /* lock is zero and available */
  m->lock = 0;
  return 0;
}

/* simultaneously tests and sets a lock 
 * common idiom.
 * */
int test_and_set(int *old_val, int new)
{
  int old = *old_val;
  *old_val = new;
  return old;
}

/* this is a yielding lock. It mostly works */
int unmthread_mutex_lock(unmmutex_t *m)
{
  while (test_and_set(&m->lock, 1) == 1) {
    unmthread_yield();
  }
  return 0;
}

int unmthread_mutex_unlock(unmmutex_t *m)
{
  m->lock = 0;
  return 0;
}

/* duplicate as there were no condition vars */
int unmthread_mutex_uninit(unmmutex_t *m)
{
  m->lock = 0;
  return 0;
}


