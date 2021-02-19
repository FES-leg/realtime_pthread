#include <iostream>
#include <stdio.h>              // printf
#include <string.h>
#include <time.h>               // cyclic loop
#include <pthread.h>            // multithread
#include <algorithm>            // std::copy
#include <signal.h>             // to catch ctrl-c
#include <unistd.h>

#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include "time_spec_operation.h"
#include "loop_time_stats.h"

#define DEFAULT_LOOP_TIME_NS 1000000L
#define DEFAULT_APP_DURATION_COUNTS 10000
#define NSEC_PER_SEC 1000000000L
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



    while(loop_count<DEFAULT_APP_DURATION_COUNTS){

        realtime_loop_time_stats.loop_starting_point();

        /* Calculate the time for the execution of this task*/
        t_period.tv_sec = 0;
        t_period.tv_nsec = DEFAULT_LOOP_TIME_NS;
        TIMESPEC_INCREMENT ( t_next, t_period );

        //        clock_gettime ( CLOCK_MONOTONIC, &t_now);

        if(loop_count%1000==0){
            timespec_sub(&t_result,&t_now,&t_start);
            PLOGI << green_key << "RT Clock: " << (double) (timespec_to_nsec(&t_result)/1e9) << color_key << endl;
            my_task();
        }
        loop_count++;

        /* Sleep until the next execution*/
        clock_nanosleep ( CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, nullptr );
        clock_gettime ( CLOCK_MONOTONIC, &t_now);

        t_overflow = (t_now.tv_sec*1e9 + t_now.tv_nsec) - (t_next.tv_sec*1e9 + t_next.tv_nsec);
        if(t_overflow > ALLOWED_LOOPTIME_OVERFLOW_NS)
        {
            cout << red_key << "RT Overflow: " << t_overflow << color_key << endl;
        }

    }

    realtime_loop_time_stats.store_loop_time_stats();
}

void *nrt(void * federica){
    /* NRT code */
    loop_time_stats non_realtime_loop_time_stats("non_realtime_loop_time_stats.txt",loop_time_stats::output_mode::screenout_only);
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender; // Create the 2nd appender.

    plog::init(plog::info,&consoleAppender);

    struct timespec t_start, t_now, t_next, t_period, t_prev,t_result;
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
        PLOGE <<  "Error: rt not created" ;
        return 1;
    }

    /** NON REAL-TIME THREAD */

    pthread_attr_init( &attr);
    pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED);
    policy = SCHED_OTHER;
    pthread_attr_setschedpolicy( &attr, policy);
    prio.sched_priority = 1; // priority range should be btw -20 to +19
    pthread_attr_setschedparam(&attr,&prio);

    if ( pthread_create(&nrt_loop, &attr, nrt, nullptr) ){
        PLOGE << "Error: nrt not created" ;
        return 1;
    }



    /* wait for threads to finish */
    pthread_join(rt_loop,NULL);
    pthread_join(nrt_loop,NULL);
    return 0;
}
