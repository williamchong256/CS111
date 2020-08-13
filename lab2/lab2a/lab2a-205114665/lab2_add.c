//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <string.h>

//global counter
long long counter;
int num_iterations;
int num_threads;
int opt_yield;
int opt_sync;
char sync_type;
pthread_mutex_t countermutex = PTHREAD_MUTEX_INITIALIZER; //statically initialize mutex

volatile int spinlockvar = 0;

//TODO:
    //takes a parameter for the number of parallel threads (--threads=#, default 1).
    //takes a parameter for the number of iterations (--iterations=#, default 1).
    //initializes a (long long) counter to zero

void spinLock()
{
    while (__sync_lock_test_and_set(&spinlockvar, 1));
}

void spinLockRelease()
{
    __sync_lock_release(&spinlockvar);
}


void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        if (opt_yield)
                sched_yield();
        *pointer = sum;
}

void* threadFunction()
{
    long long oldval, newval;
    
    //to add 1
    for (int i=0; i<num_iterations; i++)
    {
        if (opt_sync){
            switch(sync_type) {
                case 'm':
                    //mutex
                    pthread_mutex_lock(&countermutex);
                    add(&counter, 1);
                    pthread_mutex_unlock(&countermutex);
                    break;
                case 's':
                    //spinlock
                    spinLock();
                    add(&counter,1);
                    spinLockRelease();
                    break;
                case 'c':
                    //compare and swap
                    do {
                        oldval = counter;
                        newval = oldval + 1;
                        if (opt_yield)
                        {
                            sched_yield();
                        }
                    }while (__sync_val_compare_and_swap(&counter, oldval, newval) != oldval);
                    break;
            }
        }
        else {
            add(&counter, 1);
        }
        
    }
    
    //to add -1
    for (int i=0; i<num_iterations; i++)
    {
        if (opt_sync)
        {
            switch (sync_type) {
                case 'm':
                    pthread_mutex_lock(&countermutex);
                    add(&counter, -1);
                    pthread_mutex_unlock(&countermutex);
                    break;
                case 's':
                    spinLock();
                    add(&counter, -1);
                    spinLockRelease();
                    break;
                case 'c':
                    //compare and swap
                    do {
                        oldval = counter;
                        newval = oldval - 1;
                        if (opt_yield)
                        {
                            sched_yield();
                        }
                    }while (__sync_val_compare_and_swap(&counter, oldval, newval) != oldval);
                    break;
            }
        }
        else {
            add(&counter, -1);
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char * argv[]) {
    struct timespec starttime, endtime;
    num_iterations = 1;
    num_threads = 1;
    opt_yield = 0;
    opt_sync = 0;
    counter = 0;
    void* status;
    
    //option struct
    static struct option longopts[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {0,0,0,0}
    };
    int optionindex = 0;
    
    int opt = getopt_long(argc, argv, "", longopts, &optionindex);
    
    while (opt != -1) {
        switch(opt)
        {
            case 't':
                if ((num_threads = atoi(optarg)) < 0)
                {
                    fprintf(stderr, "Invalid argument to --threads.\n");
                    exit(1);
                }
                break;
            case 'i':
                if ((num_iterations = atoi(optarg)) < 0)
                {
                    fprintf(stderr, "Invalid argument to --iterations.\n");
                    exit(1);
                }
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                opt_sync = 1;
                sync_type = optarg[0];
                break;
            default:
                fprintf(stderr, "Invalid argument. Correct usage: ./lab2_add --threads=numthreads --iterations=numiterations\n");
                exit(1);
        }
        opt = getopt_long(argc, argv, "", longopts, &optionindex);
    }
    
    //get start time
    if (clock_gettime(CLOCK_REALTIME, &starttime) < 0)
    {
        fprintf(stderr, "Error getting starttime using clock_gettime().\n");
        exit(1);
    }
    
    pthread_t* threads = (pthread_t*) malloc(sizeof(pthread_t)*num_threads);
    if (threads == NULL)
    {
        fprintf(stderr, "Error allocating memory for pthreads.\n");
        exit(1);
    }
    
    //create threads
    for (int i=0; i<num_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, (void*)&threadFunction, NULL) != 0)
        {
            fprintf(stderr, "Error creating threads.\n");
            exit(1);
        }
    }
    
    //join threads
    for (int i=0; i<num_threads; i++)
    {
        if (pthread_join(threads[i], &status) != 0)
        {
            fprintf(stderr, "Error joining threads.\n");
            exit(1);
        }
    }
    
    if (clock_gettime(CLOCK_REALTIME, &endtime) < 0)
    {
        fprintf(stderr, "Error getting endtime with clock_gettime().\n");
        exit(1);
    }
    
    //total num of operations
    long num_operations = num_threads * num_iterations * 2;
    //total runtime
    long run_time = (endtime.tv_sec*1e9 + endtime.tv_nsec) - (starttime.tv_sec*1e9 + starttime.tv_nsec);
    //average time per operation
    long avg_op_time = run_time / num_operations;

    //initialize a string to store tag, zero out the memory
    char tag [30];
    memset(tag, 0, 30*sizeof(char));
    sprintf(tag, "add");
    
    
    if (opt_yield)
    {
        strcat(tag, "-yield");
        if (opt_sync)
        {
            switch (sync_type) {
                case 'm':
                    strcat(tag, "-m");
                    break;
                case 's':
                    strcat(tag, "-s");
                    break;
                case 'c':
                    strcat(tag, "-c");
                    break;
            }
        }
        else {
            strcat(tag, "-none");
        }
    }
    else
    {
        //no yield option
        if (opt_sync)
        {
            switch (sync_type) {
                case 'm':
                    strcat(tag, "-m");
                    break;
                case 's':
                    strcat(tag, "-s");
                    break;
                case 'c':
                    strcat(tag, "-c");
                    break;
            }
        }
        else {
            //add-none
            strcat(tag, "-none");
        }
    }
    
    //prints to stdout a comma-separated-value (CSV) record
    printf("%s,%d,%d,%ld,%ld,%ld,%lld\n", tag, num_threads, num_iterations, num_operations, run_time, avg_op_time, counter);
//    the name of the test (add-none for the most basic usage)
//    the number of threads (from --threads=)
//    the number of iterations (from --iterations=)
//    the total number of operations performed (threads x iterations x 2, the "x 2" factor because you add 1 first and then add -1)
//    the total run time (in nanoseconds)
//    the average time per operation (in nanoseconds).
//    the total at the end of the run (0 if there were no conflicting updates)
    
    //destroy the mutex
    pthread_mutex_destroy(&countermutex);
    pthread_exit(NULL);
    
    return 0;
}

