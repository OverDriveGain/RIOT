/*
 * Copyright (C) 2017 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_event
 * @brief       Provides functionality to trigger events after timeout
 *
 * event_timeout intentionally does't extend event structures in order to
 * support events that are integrated in larger structs intrusively.
<<<<<<< HEAD
 *
 * Example:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~ {.c}
 * event_timeout_t event_timeout;
 *
 * printf("posting timed callback with timeout 1sec\n");
 * event_timeout_init(&event_timeout, &queue, (event_t*)&event);
 * event_timeout_set(&event_timeout, 1000000);
 * [...]
 * ~~~~~~~~~~~~~~~~~~~~~~~~
 *
=======
 * Example:
 *
 *     event_timeout_t event_timeout;
 *
 *     printf("posting timed callback with timeout 1sec\n");
 *     event_timeout_init(&event_timeout, &queue, (event_t*)&event);
 *     event_timeout_set(&event_timeout, 1000000);
 *     [...]
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
 * @{
 *
 * @file
 * @brief       Event Timeout API
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 */

#ifndef EVENT_TIMEOUT_H
#define EVENT_TIMEOUT_H

#include "event.h"
#include "xtimer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Timeout Event structure
 */
typedef struct {
    xtimer_t timer;         /**< xtimer object used for timeout */
    event_queue_t *queue;   /**< event queue to post event to   */
    event_t *event;         /**< event to post after timeout    */
} event_timeout_t;

/**
 * @brief   Initialize timeout event object
 *
<<<<<<< HEAD
 * @param[in]   event_timeout   event_timeout object to initialize
 * @param[in]   queue           queue that the timed-out event will be added to
 * @param[in]   event           event to add to queue after timeout
 */
void event_timeout_init(event_timeout_t *event_timeout, event_queue_t *queue,
                        event_t *event);
=======
 * @param[in]   event_timeout   event_timeout object to initilalize
 * @param[in]   queue           queue that the timed-out event will be added to
 * @param[in]   event           event to add to queue after timeout
 */
void event_timeout_init(event_timeout_t *event_timeout, event_queue_t *queue, event_t *event);
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791

/**
 * @brief   Set a timeout
 *
 * This will make the event as configured in @p event_timeout be triggered
<<<<<<< HEAD
 * after @p timeout microseconds.
=======
 * after @p timeout miliseconds.
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
 *
 * @note: the used event_timeout struct must stay valid until after the timeout
 *        event has been processed!
 *
<<<<<<< HEAD
 * @param[in]   event_timeout   event_timout context object to use
 * @param[in]   timeout         timeout in microseconds
 */
void event_timeout_set(event_timeout_t *event_timeout, uint32_t timeout);

/**
 * @brief   Clear a timeout event
 *
 * Calling this function will cancel the timeout by removing its underlying
 * timer. If the timer has already fired before calling this function, the
 * connected event will be put already into the given event queue and this
 * function does not have any effect.
 *
 * @param[in]   event_timeout   event_timeout context object to use
 */
void event_timeout_clear(event_timeout_t *event_timeout);

=======
 * @param[in]   event_timeout   event_timout context onject to use
 * @param[in]   timeout         timeout in miliseconds
 */
void event_timeout_set(event_timeout_t *event_timeout, uint32_t timeout);

>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
#ifdef __cplusplus
}
#endif
#endif /* EVENT_TIMEOUT_H */
/** @} */
