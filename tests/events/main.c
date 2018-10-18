/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       event test application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdio.h>

#include "thread.h"
#include "event.h"
#include "event/timeout.h"
#include "event/callback.h"

<<<<<<< HEAD
static unsigned order;
static uint32_t before;

static void callback(event_t *arg);
static void custom_callback(event_t *event);
static void timed_callback(void *arg);
static void forbidden_callback(void *arg);


static event_t event = { .handler = callback };
static event_t event2 = { .handler = callback };

static void callback(event_t *arg)
{
    order++;
    assert(order == 1);
    assert(arg == &event);
    printf("triggered 0x%08x\n", (unsigned)arg);
}

=======
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
typedef struct {
    event_t super;
    const char *text;
} custom_event_t;

<<<<<<< HEAD
static custom_event_t custom_event = { .super.handler = custom_callback, .text = "CUSTOM CALLBACK" };
static event_callback_t event_callback = EVENT_CALLBACK_INIT(timed_callback, 0x12345678);
static event_callback_t noevent_callback = EVENT_CALLBACK_INIT(forbidden_callback, 0);

static void custom_callback(event_t *event)
{
    order++;
    assert(order == 2);
    assert(event == (event_t *)&custom_event);
=======
static void custom_callback(event_t *event)
{
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
    custom_event_t *custom_event = (custom_event_t *)event;
    printf("triggered custom event with text: \"%s\"\n", custom_event->text);
}

<<<<<<< HEAD
static void timed_callback(void *arg)
{
    order++;
    assert(order == 3);
    assert(arg == event_callback.arg);
    uint32_t now = xtimer_now_usec();
    assert((now - before >= 100000LU));
    printf("triggered timed callback with arg 0x%08x after %" PRIu32 "us\n", (unsigned)arg, now - before);
    puts("[SUCCESS]");
}

static void forbidden_callback(void *arg)
{
    (void)arg;
    /* this callback should never be triggered! */
    puts("call to forbidden callback");
    puts("[FAILED]");
    while (1) {
        assert(false);
    }
=======
static void callback(event_t *event)
{
    printf("triggered 0x%08x\n", (unsigned)event);
}

static void timed_callback(void *arg)
{
    printf("triggered timed callback with arg 0x%08x\n", (unsigned)arg);
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
}

int main(void)
{
<<<<<<< HEAD
    puts("[START] event test application.\n");

    event_queue_t queue = { .waiter = (thread_t *)sched_active_thread };
=======
    puts("event test application.\n");

    event_queue_t queue = { .waiter=(thread_t*)sched_active_thread };
    event_t event = { .handler=callback };
    event_t event2 = { .handler=callback };

>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
    printf("posting 0x%08x\n", (unsigned)&event);
    event_post(&queue, &event);

    printf("posting 0x%08x\n", (unsigned)&event2);
    event_post(&queue, &event2);
    printf("canceling 0x%08x\n", (unsigned)&event2);
    event_cancel(&queue, &event2);

<<<<<<< HEAD
    puts("posting custom event");
    event_post(&queue, (event_t *)&custom_event);

    event_timeout_t event_timeout;

    puts("posting timed callback with timeout 1sec");
    event_timeout_init(&event_timeout, &queue, (event_t *)&event_callback);
    before = xtimer_now_usec();
    event_timeout_set(&event_timeout, (1 * US_PER_SEC));

    event_timeout_t event_timeout_canceled;

    puts("posting timed callback with timeout 0.5sec and canceling it again");
    event_timeout_init(&event_timeout_canceled, &queue,
                       (event_t *)&noevent_callback);
    event_timeout_set(&event_timeout_canceled, 500 * US_PER_MS);
    event_timeout_clear(&event_timeout_canceled);

    puts("launching event queue");
=======
    printf("posting custom event\n");
    custom_event_t custom_event = { .super.handler=custom_callback, .text="CUSTOM CALLBACK" };
    event_post(&queue, (event_t *)&custom_event);

    event_timeout_t event_timeout;
    event_callback_t event_callback = EVENT_CALLBACK_INIT(timed_callback, 0x12345678);

    printf("posting timed callback with timeout 1sec\n");
    event_timeout_init(&event_timeout, &queue, (event_t*)&event_callback);
    event_timeout_set(&event_timeout, 1000000);

    printf("launching event queue\n");
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
    event_loop(&queue);

    return 0;
}
