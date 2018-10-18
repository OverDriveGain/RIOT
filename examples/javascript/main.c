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

<<<<<<< HEAD
int js_run(const jerry_char_t *script, size_t script_size)
{

    jerry_value_t parsed_code, ret_value;
    int res = 0;

    /* Initialize engine, no flags, default configuration */

    jerry_init(JERRY_INIT_EMPTY);

    /* Register the print function in the global object. */

    jerryx_handler_register_global((const jerry_char_t *) "print",
                                   jerryx_handler_print);

    /* Setup Global scope code */

    parsed_code = jerry_parse(NULL, 0, script, script_size, JERRY_PARSE_NO_OPTS);

    if (!jerry_value_is_error(parsed_code)) {
        /* Execute the parsed source code in the Global scope */
        ret_value = jerry_run(parsed_code);
        if (jerry_value_is_error(ret_value)) {
            printf("js_run(): Script Error!");
            res = -1;
        }
        jerry_release_value(ret_value);
    }

    jerry_release_value(parsed_code);
=======
static event_queue_t event_queue;

char script[2048];

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/* import "ifconfig" shell command, used for printing addresses */
extern int _netif_config(int argc, char **argv);

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
>>>>>>> d74552ae8de9d8b57bce6676d98c3205a040c791

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
    _netif_config(0, NULL);

    /* register to LWM2M server */
    puts("initializing coap, registering at lwm2m server");
    lwm2m_init();
    lwm2m_register();

    puts("setting up event queue");
    event_queue_init(&event_queue);
    js_event_queue = &event_queue;

    puts("Entering event loop...");
    event_loop(&event_queue);

    return 0;
}
