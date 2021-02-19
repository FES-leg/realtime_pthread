#include <iostream>
#include <stdio.h>              // printf
#include <string.h>
#include <time.h>               // cyclic loop
#include <pthread.h>            // multithread
#include <algorithm>            // std::copy
#include <signal.h>             // to catch ctrl-c
#include <unistd.h>


#include "time_spec_operation.h"
#include "loop_time_stats.h"

#define DEFAULT_LOOP_TIME_NS 1000000L
#define NSEC_PER_SEC 1000000000L
#define ALLOWED_LOOPTIME_OVERFLOW_NS 200000L


using namespace std;

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

void my_task(){
    cout << "Rehamove" << endl;
}

void* rt(void* cookie){

    loop_time_stats realtime_loop_time_stats("realtime_loop_time_stats.txt",loop_time_stats::output_mode::screenout_only);

    unsigned int period = DEFAULT_LOOP_TIME_NS;
    struct timespec t_start, t_now, t_next,t_period,t_result;
    unsigned long int t_overflow = 0;   // measure the overflowed time for each cycle
    unsigned long int loop_count = 0;
    /* ... */
    /* Initialization code */
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    clock_gettime( CLOCK_MONOTONIC, &t_now);
    t_next = t_now;



    while(loop_count<5000){

        realtime_loop_time_stats.loop_starting_point();

        /* Calculate the time for the execution of this task*/
        t_period.tv_sec = 0;
        t_period.tv_nsec = DEFAULT_LOOP_TIME_NS;
        TIMESPEC_INCREMENT ( t_next, t_period );

//        clock_gettime ( CLOCK_MONOTONIC, &t_now);

        if(loop_count%1000==0){

        cout << "Hi, my name is Real-Time! ";
        timespec_sub(&t_result,&t_now,&t_start);
        cout << (double) (timespec_to_nsec(&t_result)/1e9) << endl;
        my_task();
        }
        loop_count++;
        /* Sleep until the next execution*/

        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, nullptr );
        clock_gettime ( CLOCK_MONOTONIC, &t_now);

        t_overflow = (t_now.tv_sec*1e9 + t_now.tv_nsec) - (t_next.tv_sec*1e9 + t_next.tv_nsec);
        if(t_overflow > ALLOWED_LOOPTIME_OVERFLOW_NS)
        {
            cout << "Cycle time exceeded the desired cycle time. Overflowed time: " << t_overflow << endl;
        }

    }

    realtime_loop_time_stats.store_loop_time_stats();
}

void *nrt(void * federica){
/* NRT code */
    struct timespec t_start, t_now;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    while(1)
    {
        clock_gettime ( CLOCK_MONOTONIC, &t_now);
    cout << "Hello World non Real-Time! ";
    cout << (double) (t_now.tv_nsec/1e9+(t_now.tv_sec-t_start.tv_sec)) << endl;
    usleep(1000000);
    }
}

int main()
{
    int policy;
    struct sched_param prio;
    pthread_attr_t attr;

    pthread_t rt_loop;
    pthread_t nrt_loop;

    /** REAL-TIME THREAD */
    pthread_attr_init( &attr);
    pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED);
    policy = SCHED_RR;
    pthread_attr_setschedpolicy( &attr, policy);
    prio.sched_priority = 1; // priority range should be btw -20 to +19
    pthread_attr_setschedparam(&attr,&prio);

    if ( pthread_create(&rt_loop, &attr, rt, nullptr ) ){
        cout <<  "Error: rt not created" ;
        return 1;
    }

    /** NON REAL-TIME THREAD */

    pthread_attr_init( &attr);
    pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED);
    policy = SCHED_OTHER;
    pthread_attr_setschedpolicy( &attr, policy);
    prio.sched_priority = -20; // priority range should be btw -20 to +19
    pthread_attr_setschedparam(&attr,&prio);

    if ( pthread_create(&nrt_loop, &attr, nrt, nullptr) ){
        cout << "Error: nrt not created" ;
        return 1;
    }



    /* wait for threads to finish */
    pthread_join(rt_loop,NULL);
    pthread_join(nrt_loop,NULL);
    return 0;
}
