#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

#define PRIO_LOW 	7
#define PRIO_MED 	5
#define PRIO_HIGH	3 
#define PRIO_COOP	(-1) 


void coop_fn(void *p1, void *p2, void *p3)
{

   LOG_INF("[COOP] starting - will run 3 steps without yielding");

    for (int i = 0; i < 3; i++) {
        k_busy_wait(40000);   
        LOG_INF("[COOP] step %d/3 - still holding CPU  tick=%u",
                i + 1, k_uptime_get_32());
    }

    LOG_INF("[COOP] yielding now - HIGH, MED and LOW can run");
    k_yield();

    LOG_INF("[COOP] done");

}


void low_fn(void *p1, void *p2, void *p3)
{

   LOG_INF("[LOW] started");

   for (int i = 0; i < 10; i++) {
        LOG_INF("[LOW] step %d  tick=%u", i, k_uptime_get_32());

        k_msleep(300);
    }

    LOG_INF("[LOW] done");

}


void med_fn(void *p1, void *p2, void *p3)
{

   LOG_INF("[MED] started");

   for (int i = 0; i < 10; i++) {
        LOG_INF("[MED] step %d  tick=%u", i, k_uptime_get_32());

        k_msleep(200);
    }

    LOG_INF("[MED] done");

}


void high_fn(void *p1, void *p2, void *p3)
{

   LOG_INF("[HIGH] started");

   for (int i = 0; i < 10; i++) {
        LOG_INF("[HIGH] step %d  tick=%u", i, k_uptime_get_32());

        k_msleep(100);
    }

    LOG_INF("[HIGH] done");

}


K_THREAD_DEFINE(t_coop, STACK_SIZE, coop_fn,
                NULL, NULL, NULL, PRIO_COOP, 0, 0);

K_THREAD_DEFINE(t_low, STACK_SIZE, low_fn,
                NULL, NULL, NULL, PRIO_LOW, 0, 0);

K_THREAD_DEFINE(t_med, STACK_SIZE, med_fn,
                NULL, NULL, NULL, PRIO_MED, 0, 0);

K_THREAD_DEFINE(t_high, STACK_SIZE,high_fn,
                NULL, NULL, NULL, PRIO_HIGH, 0, 0);


int main(void)
{

    LOG_INF("=== L1 Task 1: Scheduling Competition ===");
    LOG_INF("COOP prio=%d (cooperative)  HIGH prio=%d  MED prio=%d  LOW prio=%d",
            PRIO_COOP, PRIO_HIGH, PRIO_MED, PRIO_LOW);

    return 0;

}

