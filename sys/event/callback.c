/*
 * Copyright (C) 2017 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "event/callback.h"

void _event_callback_handler(event_t *event)
{
    event_callback_t *event_callback = (event_callback_t *) event;
    event_callback->callback(event_callback->arg);
}

<<<<<<< HEAD
void event_callback_init(event_callback_t *event_callback, void (callback)(void *), void *arg)
=======
void event_callback_init(event_callback_t *event_callback, void (callback)(void*),  void *arg)
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791
{
    event_callback->super.handler = _event_callback_handler;
    event_callback->callback = callback;
    event_callback->arg = arg;
}
