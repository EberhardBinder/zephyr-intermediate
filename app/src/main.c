#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(demo, LOG_LEVEL_DBG);

#define STACK_SIZE 1024

/* ================================================================== */
/*  Shared message type                                               */
/* ================================================================== */

struct sensor_msg {
    uint32_t timestamp_ms;
    int32_t value;
    uint8_t seq;
};

/* ================================================================== */
/*           Thread-to-thread pipeline                                */
/* ================================================================== */

#define SENSOR_QUEUE_DEPTH 6
#define COUNT       12

K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_msg), SENSOR_QUEUE_DEPTH, 4);

static K_SEM_DEFINE(prod_done, 0, 1);
static K_SEM_DEFINE(cons_done, 0, 1);

static volatile bool prod_finished;

void my_callback()  {

    LOG_INF("my_callback");

}

static void producer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "prod");

    for (int i = 0; i < COUNT; i++) {
        struct sensor_msg msg = {
            .timestamp_ms = k_uptime_get_32(),
            .value = 100 + i,
            .seq = (uint8_t)i,
        };

        int ret = k_msgq_put(&sensor_queue, &msg, K_MSEC(200));
        if (ret == 0) {
            LOG_INF("[PROD] sent seq=%u val=%d q=%u/%d",
                    msg.seq,
                    msg.value,
                    k_msgq_num_used_get(&sensor_queue),
                    SENSOR_QUEUE_DEPTH);
        } else {
            LOG_WRN("[PROD] put failed ret=%d", ret);
        }

        k_msleep(50);
    }

    prod_finished = true;
    LOG_INF("[PROD] done");
    k_sem_give(&prod_done);
}

static void consumer(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    k_thread_name_set(k_current_get(), "cons");

    /* Register a channel per monitored thread */
    int chan = task_wdt_add(2000,        /* timeout ms */
                            my_callback, /* called if thread misses feed */
                            (void *)k_current_get());

    while (true) {
        struct sensor_msg msg = {0};

        int ret = k_msgq_get(&sensor_queue, &msg, K_MSEC(300));
        if (ret != 0) {
            if (prod_finished && k_msgq_num_used_get(&sensor_queue) == 0) {
                break;
            }

            LOG_WRN("[CONS] timeout waiting for message");
            continue;
        }

        uint32_t latency = k_uptime_get_32() - msg.timestamp_ms;

        LOG_INF("[CONS] got seq=%u val=%d q=%u/%d latency=%ums",
                msg.seq,
                msg.value,
                k_msgq_num_used_get(&sensor_queue),
                SENSOR_QUEUE_DEPTH,
                latency);

        task_wdt_feed(chan); /* In the thread: feed regularly */

        k_msleep(5000);
    }

    LOG_INF("[CONS] done");
    k_sem_give(&cons_done);
}

/* ================================================================== */
/*  Health monitor                                                    */
/* ================================================================== */

static void health_fn(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);

    for (int i = 0; i < 6; i++) {
        k_msleep(120);

        uint32_t used = k_msgq_num_used_get(&sensor_queue);

        LOG_INF("[HEALTH] sensor_queue=%u/%u",
            used, SENSOR_QUEUE_DEPTH);

        LOG_DBG("[HEALTH] queue has %u free slots", SENSOR_QUEUE_DEPTH - used);

#if ENABLE_AUDIT_QUEUE
        LOG_INF("[HEALTH] audit_queue=%u/16", k_msgq_num_used_get(&audit_queue));
#endif
    }

    LOG_INF("[HEALTH] done");
}


/* ================================================================== */
/*  Runtime threads                                                   */
/* ================================================================== */

K_THREAD_STACK_DEFINE(prod_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(cons_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(health_fn_stack, STACK_SIZE);

static struct k_thread prod_thread;
static struct k_thread cons_thread;
static struct k_thread health_thread;

/* ================================================================== */
/*  Main                                                              */
/* ================================================================== */

int main(void)
{
    LOG_INF("=== L5 Task 1: Message Queue Pipeline ===");
    LOG_INF("sizeof(sensor_msg)=%u", sizeof(struct sensor_msg));

    LOG_INF("\n--- thread-to-thread pipeline ---");

    k_msgq_purge(&sensor_queue);
    prod_finished = false;

    task_wdt_init(NULL); /* NULL = software-only; pass hw wdt device for fallback */


    k_thread_create(&health_thread, health_fn_stack,
                    K_THREAD_STACK_SIZEOF(health_fn_stack), health_fn, 
                    NULL, NULL, NULL, 5, 0, K_NO_WAIT);


    k_thread_create(&prod_thread, prod_stack,
                    K_THREAD_STACK_SIZEOF(prod_stack), producer, 
                    NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    /*
    * Let producer get ahead.
    * This makes the queue buffer real messages instead of direct handoff.
    */
    k_msleep(180);

    k_thread_create(&cons_thread,
                    cons_stack,
                    K_THREAD_STACK_SIZEOF(cons_stack), consumer,
                    NULL, NULL, NULL, 5, 0, K_NO_WAIT);

    k_sem_take(&prod_done, K_FOREVER);
    k_sem_take(&cons_done, K_FOREVER);

    k_msleep(200);

    LOG_INF("\n=== Demo complete ===");

    return 0;
}
