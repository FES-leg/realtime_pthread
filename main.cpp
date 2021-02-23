#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>               // cyclic loop
#include <pthread.h>            // multithread
#include <algorithm>            // std::copy
#include <signal.h>             // to catch ctrl-c

#ifdef _POSIX_PRIORITY_SCHEDULING
#define POSIX "POSIX 1003.1b\n";
#endif
#ifdef _POSIX_THREADS
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
#define POSIX "POSIX 1003.1c\n";
#endif
#endif

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include "time_spec_operation.h"
#include "loop_time_stats.h"

#define DEFAULT_LOOP_TIME_NS 1000000L
#define DEFAULT_APP_DURATION_COUNTS 10000
#define ALLOWED_LOOPTIME_OVERFLOW_NS 200000L

// Text Color Identifiers
const string boldred_key = "\033[1;31m";
const string red_key = "\033[31m";
const string boldpurple_key = "\033[1;35m";
const string yellow_key = "\033[33m";
const string blue_key = "\033[36m";
const string green_key = "\033[32m";
const string color_key = "\033[0m";

using namespace std;

/* Cyclic Loop Task */
void my_task(){
    // my_task
}

static
void getinfo ()
{
    struct sched_param param;
    int policy;

    sched_getparam(0, &param);
    printf("Priority of this process: %d\n\r", param.sched_priority);

    pthread_getschedparam(pthread_self(), &policy, &param);

    printf("Priority of the thread: %d, current policy is: %d\n\r",
              param.sched_priority, policy);
}

void* rt(void* cookie){

    loop_time_stats realtime_loop_time_stats("realtime_loop_time_stats.txt",loop_time_stats::output_mode::screenout_only);

    struct timespec t_start, t_now, t_next,t_period,t_result; // timespec variable to handle timing
    unsigned long int t_overflow = 0;   // measure the overflowed time for each cycle
    unsigned long int loop_count = 0;   // counter for cycle loops

    getinfo();

    /* Initialization code */
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    clock_gettime( CLOCK_MONOTONIC, &t_now);
    t_next = t_now;

    /* Cyclic Loop */
    while(loop_count<DEFAULT_APP_DURATION_COUNTS){

        // Save timing stats
        realtime_loop_time_stats.loop_starting_point();

        // Calculate the time period for the execution of this task
        t_period.tv_sec = 0;
        t_period.tv_nsec = DEFAULT_LOOP_TIME_NS;

        // Compute next cycle deadline
        TIMESPEC_INCREMENT ( t_next, t_period );

        // Debug cycle at 1 Hz
        if(loop_count%1000==0){

            // Compute time since start
            timespec_sub(&t_result,&t_now,&t_start);
            PLOGI << green_key << "RT Clock: " << (double) (timespec_to_nsec(&t_result)/1e9) << color_key << endl;

            // Run my cyclic task
            my_task();
        }
        loop_count++;

        // Sleep until the next execution time
        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, nullptr );
        // Get time after sleep
        clock_gettime ( CLOCK_MONOTONIC, &t_now);
        // Compute overflow time
        t_overflow = (t_now.tv_sec*1e9 + t_now.tv_nsec) - (t_next.tv_sec*1e9 + t_next.tv_nsec);
        // If overflow is too big, print overrun
        if(t_overflow > ALLOWED_LOOPTIME_OVERFLOW_NS)
        {
            cout << red_key << "RT Overflow: " << t_overflow << color_key << endl;
        }

    }

    // Print histogram on screen
    realtime_loop_time_stats.store_loop_time_stats();
}

void *nrt(void * federica){
    /* NRT code */
    loop_time_stats non_realtime_loop_time_stats("non_realtime_loop_time_stats.txt",loop_time_stats::output_mode::screenout_only);

    struct timespec t_start, t_now, t_next, t_period, t_prev,t_result;

    getinfo();


    clock_gettime(CLOCK_REALTIME, &t_start);
    t_next = t_start;
    unsigned long int loop_count = 0;
    unsigned long int t_overflow = 0;

    /* Calculate the time for the execution of this task*/
    t_period.tv_sec = 0;
    t_period.tv_nsec = DEFAULT_LOOP_TIME_NS;

    while(loop_count<DEFAULT_APP_DURATION_COUNTS*2)
    {
        t_prev = t_next;
        non_realtime_loop_time_stats.loop_starting_point();
        if(loop_count%1000==0){
            timespec_sub(&t_result,&t_now,&t_start);
            PLOGI << yellow_key << "NRT Clock: " << (double) (timespec_to_nsec(&t_result)/1e9) << color_key << endl;
        }
        loop_count++;
        timespec_add_nsec(&t_next, &t_prev, DEFAULT_LOOP_TIME_NS);
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t_next, NULL);
        clock_gettime ( CLOCK_REALTIME, &t_now);

        t_overflow = (t_now.tv_sec*1e9 + t_now.tv_nsec) - (t_next.tv_sec*1e9 + t_next.tv_nsec);
        if(t_overflow > ALLOWED_LOOPTIME_OVERFLOW_NS)
        {
            cout << red_key << "NRT Overflow: " << t_overflow << color_key << endl;
        }
    }

    non_realtime_loop_time_stats.store_loop_time_stats();

}

int main()
{
    // Plogger initialization
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender; // Create appender.
    plog::init(plog::info,&consoleAppender);
    PLOGI << "Version " << POSIX;

    printf("Max priority for SCHED_OTHER %d\n", sched_get_priority_max(SCHED_OTHER));
    printf("Min priority for SCHED_OTHER %d\n", sched_get_priority_min(SCHED_OTHER));
    printf("Max priority for SCHED_FIFO %d\n", sched_get_priority_max(SCHED_FIFO));
    printf("Min priority for SCHED_FIFO %d\n", sched_get_priority_min(SCHED_FIFO));
    printf("Max priority for SCHED_RR %d\n", sched_get_priority_max(SCHED_RR));
    printf("Min priority for SCHED_RR %d\n", sched_get_priority_min(SCHED_RR));

    // Scheduler variables
    int policy;
    struct sched_param prio;
    pthread_attr_t attr;

    // Pthreads variables
    pthread_t rt_loop;
    pthread_t nrt_loop;

    policy = SCHED_OTHER;
    if (pthread_setschedparam( pthread_self(),policy, &prio )){
            perror ("Error: RT pthread_setschedparam (root permission?)");
            exit(1);
        }

    /********************/
    /* REAL-TIME THREAD */
    /********************/



    pthread_attr_init( &attr);
    pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED);
    policy = SCHED_FIFO;
    pthread_attr_setschedpolicy( &attr, policy);
    prio.sched_priority = 1 ; // priority range should be btw 0 and 99
    pthread_attr_setschedparam(&attr,&prio);

    if ( pthread_create(&rt_loop, &attr, rt, nullptr ) ){
        perror( "Error: RT pthread_create") ;
        return 1;
    }

    /************************/
    /* NON REAL-TIME THREAD */
    /************************/


    pthread_attr_init( &attr);
    pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED);
    policy = SCHED_OTHER;
    pthread_attr_setschedpolicy( &attr, policy);
    prio.sched_priority = 1; // priority range should be btw -20 to +19
    pthread_attr_setschedparam(&attr,&prio);

    if ( pthread_create(&nrt_loop, &attr, nrt, nullptr) ){
        perror( "Error: NRT pthread_create") ;
        return 1;
    }

    /* wait for threads to finish */
    pthread_join(rt_loop,NULL);
    pthread_join(nrt_loop,NULL);
    return 0;
}
