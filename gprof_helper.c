/*
 * bailing, 2010/01/07
 *
 * pthread_create wrapper,
 * for gprof - multithread
 *
 * 1. gcc -shared -fPIC gprof_helper.c -o ghelper.so -lpthread -ldl
 * 2. gcc prog.c -lpthread
 * 3. LD_PRELOAD=./ghelper.so ./a.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>

static void* wrapper_routine(void*);

static int (*pthread_create_orig)(
        pthread_t* __restrict,
        __const pthread_attr_t* __restrict,
        void* (*) (void*),
        void* __restrict
        ) = NULL;

void wooinit(void) __attribute__((constructor));

void wooinit(void)
{
    pthread_create_orig = dlsym(RTLD_NEXT, "pthread_create");
    fprintf(stderr, "pthread: using profiling hooks for gprof\n");
    if(pthread_create_orig == NULL){
        char* err = dlerror();
        if(err == NULL){
            err = "pthread_create is NULL";
        }
        fprintf(stderr, "%s\n", err);
        exit(EXIT_FAILURE);
    }
}

typedef struct wrapper_s
{
    void* (*start_routine)(void*);
    void* arg;

    pthread_mutex_t lock;
    pthread_cond_t cond;

    struct itimerval itimer;
}wrapper_t;

static void* wrapper_routine(void* data)
{
    void* (*start_routine)(void*) = ((wrapper_t*)data)->start_routine;
    void* arg = ((wrapper_t*)data)->arg;

    setitimer(ITIMER_PROF, &((wrapper_t*)data)->itimer, NULL);

    pthread_mutex_lock(&((wrapper_t*)data)->lock);
    pthread_cond_signal(&((wrapper_t*)data)->cond);
    pthread_mutex_unlock(&((wrapper_t*)data)->lock);

    fprintf(stderr, "pthread wrapper, start real routine\n");
    return start_routine(arg);
}

int pthread_create(
        pthread_t* __restrict thread,
        __const pthread_attr_t* __restrict attr,
        void* (*start_routine)(void*),
        void* __restrict arg
        )
{
    wrapper_t data;
    int ret;

    /* Initialize the wrapper structure */
    data.start_routine = start_routine;
    data.arg = arg;
    getitimer(ITIMER_PROF, &data.itimer);

    pthread_cond_init(&data.cond, NULL);
    pthread_mutex_init(&data.lock, NULL);


    pthread_mutex_lock(&data.lock);
    ret = pthread_create_orig(thread, attr, &wrapper_routine, &data);   /* real pthread_create call */

    /* If the thread was successfully spawned,
     * wait for the data to be released */
    if(ret == 0){
        pthread_cond_wait(&data.cond, &data.lock);
    }
    pthread_mutex_unlock(&data.lock);

    pthread_mutex_destroy(&data.lock);
    pthread_cond_destroy(&data.cond);

    return ret;
}
