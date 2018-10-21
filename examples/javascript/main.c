/*
 * Copyright (C) 2017 Inria
 *               2017 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example of how to use javascript on RIOT
 *
 * @author      Emmanuel Baccelli <emmanuel.baccelli@inria.fr>
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "msg.h"
#include "xtimer.h"
#include "lwm2m.h"
#include "js.h"

/* include headers generated from *.js */
#include "lib.js.h"
#include "local.js.h"
#include "main.js.h"

static event_queue_t event_queue;

char script[2048];

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* import "ifconfig" shell command, used for printing addresses */
extern int _gnrc_netif_config(int argc, char **argv);

void js_start(event_t *unused)
{
    (void)unused;

    size_t script_len = strlen(script);
    if (script_len) {
        puts("Initializing jerryscript engine...");
        js_init();

        puts("Executing lib.js...");
        js_run(lib_js, lib_js_len);

        puts("Executing local.js...");
        js_run(local_js, local_js_len);

        puts("Executing script...");
        js_run((jerry_char_t*)script, script_len);
    }
    else {
        puts("Emtpy script, not executing.");
    }
}

static event_t js_start_event = { .handler=js_start };

void js_restart(void)
{
    js_shutdown(&js_start_event);
}

int main(void)
{
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("waiting for network config");
    xtimer_sleep(3);

    /* print network addresses */
    puts("Configured network interfaces:");
    _gnrc_netif_config(0, NULL);

    /* register to LWM2M server */
    puts("initializing coap, registering at lwm2m server");
    lwm2m_init();
    lwm2m_register();

    puts("setting up event queue");
    event_queue_init(&event_queue);
    js_event_queue = &event_queue;

    puts("Initializing jerryscript...");
    js_event_queue = &event_queue;
    js_init();

    puts("Executing lib.js...");
    js_run(lib_js, lib_js_len);

    puts("Executing main.js...");
    js_run(main_js, main_js_len);

    puts("Entering event loop...");
    event_loop(&event_queue);

    return 0;
}