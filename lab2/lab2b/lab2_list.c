//NAME: William Chong
//EMAIL: williamchong256@gmail.com
//ID: 205114665

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "SortedList.h"

SortedList_t* sublists;
SortedListElement_t* elements;


int num_iterations;
int num_threads;
int num_lists;
int opt_yield;
char yield_type;
int opt_sync;
char sync_type;
pthread_mutex_t* mutexlocks;

int* spinlocks;

void segHandler()
{
    fprintf(stderr, "Segmentation fault, exiting...\n");
    exit(1);
}

//generate random keys with word size of 7
char* generateRandomKey()
{
    char* randkey = (char*)malloc(8*sizeof(char));
    for (int i=0; i<7; i++)
    {
        randkey[i] = 'a' + rand()%26;
    }
    randkey[7] = '\0'; //terminate with null character
    return randkey;
}

void createSublistsAndLocks()
{
    //initialize num_lists number of sublists.
    sublists = (SortedList_t *) malloc( num_lists * sizeof(SortedList_t));
    if (sublists == NULL)
    {
        fprintf(stderr, "Error allocating memory for sublists array.\n");
        exit(1);
    }
    
    //create head for each (empty) list
    for (int i=0; i<num_lists; i++)
    {
        //insert header for each list
        (sublists+i)->prev = (sublists+i);
        (sublists+i)->next = (sublists+i);
        (sublists+i)->key = NULL;
        
    }
    
    //for each sublist, create corresponding locks
    //  - initialize lock arrray
    //  - init mutex
    if (sync_type == 'm')
    {
        mutexlocks = (pthread_mutex_t*) malloc( num_lists * sizeof(pthread_mutex_t));
        if (mutexlocks == NULL)
        {
            fprintf(stderr, "Error allocating memory for mutex locks.\n");
            exit(1);
        }
        //initialize each mutex
        for (int i=0; i<num_lists; i++)
        {
            if (pthread_mutex_init((mutexlocks+i), NULL) < 0)
            {
                fprintf(stderr, "Error initiating mutex lock.\n");
                exit(1);
            }
        }
    }
    else if (sync_type == 's')
    {
        spinlocks = (int*) malloc( num_lists * sizeof(int));
        if (spinlocks==NULL)
        {
            fprintf(stderr, "Error allocating memory for spinlocks.\n");
            exit(1);
        }
        //set each spinlock to 0
        for (int i=0; i<num_lists; i++)
        {
            spinlocks[i] = 0;
        }
    }
}

int hashFunction(const char* key)
{
    //hash based on the first char in the key
    return (key[0] % num_lists);
}

//each thread
//starts with a set of pre-allocated and initialized elements (--iterations=#)
//inserts them all into a (single shared-by-all-threads) list
//gets the list length
//looks up and deletes each of the keys it had previously inserted
//exits to re-join the parent thread
void* threadFunction(void *arg)
{
    struct timespec lockstart, lockend;
    long locktime=0;
    int hashIndex;
    //passed in argument is the elements array starting at the corresponding thread's offset
    SortedListElement_t * threadElements = arg;
    
    //insert all of them into the global list
    for (int i=0; i<num_iterations; i++)
    {
        //pass the element's key through the hash function to determine what hash.
        hashIndex = hashFunction(threadElements[i].key);
        //acquire specific lock corresponding to hash
        if (sync_type == 'm')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            
            pthread_mutex_lock(mutexlocks+hashIndex);  //acquire corresponding mutex lock
            
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
        else if (sync_type == 's')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            
            while (__sync_lock_test_and_set((spinlocks+hashIndex), 1));  //acquire spinlock
            
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
        
        //insert into sublist based on which hash index
        SortedList_insert((sublists + hashIndex), (SortedListElement_t*) (threadElements+i));
        
        //release mutex or spin lock
        if (sync_type == 'm')
        {
            pthread_mutex_unlock(mutexlocks+hashIndex);  //release mutex lock
        }
        else if (sync_type == 's')
        {
            __sync_lock_release(spinlocks+hashIndex);  //release spinlock;
        }
    }
    
    
    //acquire all locks so no threads can access while accessing entire sublist
    for (int i=0; i<num_lists; i++)
    {
        if (sync_type == 'm')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            pthread_mutex_lock(mutexlocks+i);  //acquire mutex lock
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
        else if (sync_type == 's')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            while (__sync_lock_test_and_set(spinlocks+i, 1));  //acquire lock with spinLock;
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
    }
    
    //get total list length (all sublists)
    int list_len=0;
    
    for (int i=0; i<num_lists; i++)
    {
        list_len += SortedList_length(sublists+i);
    }
    
    if (list_len<0)   //corrupted list
    {
        fprintf(stderr, "Corrupted list detected.\n");
        exit(2);
    }
    else if (list_len < num_iterations)  //if length does not match up
    {
        fprintf(stderr, "Incorrect number of elements inserted into list.\n");
        exit(2);
    }
    
    //release all mutexlocks or spin locks
    for (int i=0; i<num_lists; i++)
    {
        if (sync_type == 'm')
        {
            pthread_mutex_unlock(mutexlocks + i);  //release mutex lock
        }
        else if (sync_type == 's')
        {
            __sync_lock_release(spinlocks+i);
        }
    }
    
    
    
    //look up and delete each of keys inserted
    for (int i=0; i<num_iterations; i++)
    {
        //get hash index again
        hashIndex = hashFunction(threadElements[i].key);
        
        //acquire lock
        if (sync_type == 'm')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            pthread_mutex_lock(mutexlocks+hashIndex);  //acquire mutex lock
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
        else if (sync_type == 's')
        {
            if (clock_gettime(CLOCK_REALTIME, &lockstart) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            while (__sync_lock_test_and_set(spinlocks+hashIndex, 1));
            if (clock_gettime(CLOCK_REALTIME, &lockend) < 0)
            {
                fprintf(stderr, "Error getting endtime with clock_gettime().\n");
                exit(1);
            }
            //get the time spent getting lock.
            locktime += (lockend.tv_sec*1e9 + lockend.tv_nsec) - (lockstart.tv_sec*1e9 + lockstart.tv_nsec);
        }
        
        if ( SortedList_lookup(sublists+hashIndex, (threadElements+i)->key) == NULL)
        {
            //failed lookup
            fprintf(stderr, "Not able to find key: %s in list.\n",(threadElements+i)->key );
            exit(2);
        }

        if ( SortedList_delete(threadElements+i) != 0 ) //if corrupted
        {
            fprintf(stderr, "Corrupt list element detected. Failed delete.\n");
            exit(2);
        }
        
        //release mutex or spin lock
        if (sync_type == 'm')
        {
            pthread_mutex_unlock(mutexlocks+hashIndex);  //release mutex lock
        }
        else if (sync_type == 's')
        {
            __sync_lock_release(spinlocks+hashIndex);  //release spinlock
        }
    }
    return (void*)locktime;
}

int main(int argc, char * argv[]) {
    struct timespec starttime, endtime;
    num_iterations = 1;
    num_threads = 1;
    num_lists = 1;
    opt_yield = 0;
    opt_sync = 0;
    
    signal(SIGSEGV, segHandler);
    
    //option struct
    static struct option longopts[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"lists", required_argument, 0, 'l'},
        {"yield", required_argument, 0, 'y'},
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
            case 'l':
                if ((num_lists = atoi(optarg)) < 0)
                {
                    fprintf(stderr, "Invalid argument to --lists.\n");
                    exit(1);
                }
                break;
            case 'y':
                for (size_t i=0; i<strlen(optarg); i++)
                {
                    switch (optarg[i])
                    {
                        case 'i':
                            opt_yield |= INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield |= DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield |= LOOKUP_YIELD;
                            break;
                        default:
                            fprintf(stderr, "Incorrect yield option. use --yield=[idl]\n");
                            exit(2);
                    }
                }
                break;
            case 's':
                opt_sync = 1;
                sync_type = optarg[0];
                break;
            default:
                fprintf(stderr, "Invalid argument. Correct usage: ./lab2_list --threads=numthreads --iterations=numiterations --lists=numlists --yield=[idl] --sync=[m,s]\n");
                exit(1);
        }
        opt = getopt_long(argc, argv, "", longopts, &optionindex);
    }
    
    //creates sublists and corresponding locks (if option flagged)
    createSublistsAndLocks();
    
    
//    creates and initializes (with random keys) the required number (threads x iterations) of list elements.
    //total num of elements
    long num_elements = num_threads * num_iterations;
    //allocate space for elements
    elements = (SortedListElement_t *) malloc(num_elements*sizeof(SortedListElement_t));
    if (elements == NULL)
    {
        fprintf(stderr, "Error allocating memory for elements.\n");
        exit(1);
    }
    
    //generate random key and set the keys for each element
    for (int i=0; i<num_elements; i++)
    {
        elements[i].key = generateRandomKey();
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
        if (pthread_create(&threads[i], NULL, (void*)&threadFunction, (void*)(elements+(i*num_iterations))) != 0) //pass in elements[offset]
        {
            fprintf(stderr, "Error creating threads.\n");
            exit(1);
        }
    }
    
    //for each thread's lock time
    long total_lock_time = 0;
    void** lock_time = (void*) malloc(sizeof(void**));
    
    //join threads and get each thread's lock time
    for (int i=0; i<num_threads; i++)
    {
        if (pthread_join(threads[i], lock_time) != 0)
        {
            fprintf(stderr, "Error joining threads.\n");
            exit(1);
        }
        total_lock_time += (long) *lock_time;
    }
    
    //get the end time
    if (clock_gettime(CLOCK_REALTIME, &endtime) < 0)
    {
        fprintf(stderr, "Error getting endtime with clock_gettime().\n");
        exit(1);
    }
    
    //check length of list to see if 0
    long length = 0;
    for (int i=0; i<num_lists; i++)
    {
        length += SortedList_length(sublists+i);
    }
    if ( length != 0)
    {
        fprintf(stderr, "Length of list is not 0, length is %ld.\n", length );
        exit(2);
    }
    
    
    //    the total number of operations performed: threads x iterations x 3 (insert + lookup + delete)
    long num_operations = num_threads * num_iterations * 3;
    //total runtime
    long run_time = (endtime.tv_sec*1e9 + endtime.tv_nsec) - (starttime.tv_sec*1e9 + starttime.tv_nsec);
    //average time per operation
    long avg_op_time = run_time / num_operations;
    long avg_lock_time = total_lock_time / num_operations;

    //initialize a string to store tag, zero out the memory
    //tag format:  "list-yieldopts-syncopts"
        //yieldopts = {none, i,d,l,id,il,dl,idl}
        //syncopts = {none,s,m}
    char tag [30];
    memset(tag, 0, 30*sizeof(char));
    sprintf(tag, "list");
   
    //note: yield options are computed with bitwise &..
        //    INSERT_YIELD    0x01
        //    DELETE_YIELD    0x02
        //    LOOKUP_YIELD    0x04
    switch (opt_yield) {
        case 0:
            //no yield option specified
            strcat(tag, "-none");
            break;
        case 1:
            //0x01   or 'insert'
            strcat(tag, "-i");
            break;
        case 2:
            //0x02 or 'delete'
            strcat(tag, "-d");
            break;
        case 4:
            //0x04 or 'lookup'
            strcat(tag, "-l");
            break;
        case 3:
            //0x01 | 0x02  = 'id'
            strcat(tag, "-id");
            break;
        case 5:
            //0x01 | 0x04  'il'
            strcat(tag, "-il");
            break;
        case 6:
            //0x02 | 0x04  'dl'
            strcat(tag, "-dl");
            break;
        case 7:
            strcat(tag, "-idl");
            break;
        default:
            fprintf(stderr, "Unrecognized option.\n");
            break;
    }
   
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
        }
    }
    else {
        //add-none
        strcat(tag, "-none");
    }
    
    //print csv output
//    the number of threads (from --threads=)
//    the number of iterations (from --iterations=)
//    the number of lists (always 1 in this project)
//    the total number of operations performed: threads x iterations x 3 (insert + lookup + delete)
//    the total run time (in nanoseconds) for all threads
//    the average time per operation (in nanoseconds).
    printf("%s,%d,%d,%d,%ld,%ld,%ld,%ld\n", tag, num_threads, num_iterations, num_lists, num_operations, run_time, avg_op_time, avg_lock_time);

    //free the element keys
    for (int i=0; i<num_elements; i++)
    {
        free((void*)elements[i].key);
    }
    //free elements structure
    free(elements);
    //free the list
    free(sublists);
    free(lock_time);
    free(threads);
    
    //clean up locks
    if (sync_type == 'm')
    {
        //destroy each mutex lock then free array
        for(int i=0; i<num_lists; i++)
        {
            pthread_mutex_destroy(mutexlocks+i);
        }
        free(mutexlocks);
    }
    else if (sync_type == 's')
    {
        free(spinlocks);
    }
       
    exit(0);
}

