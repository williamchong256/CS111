//
//  main.c
//  lab2_add
//
//  Created by William Chong on 5/8/20.
//  Copyright © 2020 William Chong. All rights reserved.
//

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

//global counter
long long counter;

//TODO:
    //takes a parameter for the number of parallel threads (--threads=#, default 1).
    //takes a parameter for the number of iterations (--iterations=#, default 1).
    //initializes a (long long) counter to zero


void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        *pointer = sum;
}

void threadFunction(void* iterations)
{
    add(&counter, 1);
    add(&counter, -1);
}

int main(int argc, char * argv[]) {
    struct timespec starttime, endtime;
    int num_threads = 1;
    int num_iterations = 1;
    counter = 0;
    void* status;
    
    //option struct
    static struct option longopts[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
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
        if (pthread_create(&threads[i], NULL, (void*)&threadFunction, (void*)&num_iterations) != 0)
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
    int run_time = (endtime.tv_sec*1e9 + endtime.tv_nsec) - (starttime.tv_sec*1e9 + starttime.tv_nsec);
    //average time per operation
    int avg_op_time = run_time / num_operations;

    //prints to stdout a comma-separated-value (CSV) record
    printf("add-none,%d,%d,%ld,%d,%d,%lld\n", num_threads, num_iterations, num_operations, run_time, avg_op_time, counter);
//    the name of the test (add-none for the most basic usage)
//    the number of threads (from --threads=)
//    the number of iterations (from --iterations=)
//    the total number of operations performed (threads x iterations x 2, the "x 2" factor because you add 1 first and then add -1)
//    the total run time (in nanoseconds)
//    the average time per operation (in nanoseconds).
//    the total at the end of the run (0 if there were no conflicting updates)
    
    
    return 0;
}

