#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mutex.h"
#include "event.h"
#include "js.h"
#include "net/gcoap.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static mutex_t _gcoap_mutex = MUTEX_INIT;

static struct {
    coap_pkt_t *pkt;
    uint8_t *buf;
    size_t len;
} _gcoap_req;

typedef struct {
    event_t event;
    coap_resource_t resource;
    gcoap_listener_t listener;
    jerry_value_t callback;
    js_native_ref_t ref;
} js_coap_handler_t;

static ssize_t _js_coap_resource_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    js_coap_handler_t *js_coap_handler = context;

    DEBUG("%s:l%u:%s()\n", __FILE__, __LINE__, __func__);
    mutex_lock(&_gcoap_mutex);
    DEBUG("%s:l%u:%s() delegating...\n", __FILE__, __LINE__, __func__);

    _gcoap_req.pkt = pkt;
    _gcoap_req.buf = buf;
    _gcoap_req.len = len;

    event_post(js_event_queue, &js_coap_handler->event);

    mutex_lock(&_gcoap_mutex);
    DEBUG("%s:l%u:%s() back\n", __FILE__, __LINE__, __func__);
    mutex_unlock(&_gcoap_mutex);

    return _gcoap_req.len;
}

static void _js_coap_handler_event_cb(event_t *event)
{
    jerry_value_t object = 0;
    jerry_value_t methods_val = 0;
    jerry_value_t payload_val = 0;
    jerry_value_t ret_val = 0;
    jerry_value_t reply_payload_val = 0;
    unsigned reply_code = COAP_CODE_INTERNAL_SERVER_ERROR;

    void *reply_buf = NULL;
    size_t reply_len = 0;

    DEBUG("%s:l%u:%s()\n", __FILE__, __LINE__, __func__);
    js_coap_handler_t *js_coap_handler = (js_coap_handler_t *) event;

    object = jerry_create_object();
    if (jerry_value_has_error_flag(object)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        goto error;
    }

    methods_val = jerry_create_number(coap_method2flag(coap_get_code_detail(_gcoap_req.pkt)));
    if (jerry_value_has_error_flag(methods_val)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        goto error;
    }

    if (_gcoap_req.pkt->payload_len) {
        payload_val = jerry_create_string_sz((jerry_char_t *)_gcoap_req.pkt->payload, _gcoap_req.pkt->payload_len);
        if (jerry_value_has_error_flag(payload_val)) {
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
            goto error;
        }
        js_add_object(object, payload_val, "payload");
    }

    jerry_value_t args[] = { methods_val, payload_val };

    ret_val = jerry_call_function(js_coap_handler->callback, object, args, payload_val ? 2 : 1);

    if (jerry_value_has_error_flag(ret_val)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        goto error;
    }

    jerry_release_value(payload_val);
    payload_val = 0;

    reply_payload_val = js_get_property(object, "reply");

    reply_code = jerry_get_number_value(ret_val);
    switch(reply_code) {
        case COAP_CODE_CREATED:
        case COAP_CODE_DELETED:
        case COAP_CODE_VALID:
        case COAP_CODE_CHANGED:
            break;
        case COAP_CODE_CONTENT:
        default:
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
            reply_code = COAP_CODE_INTERNAL_SERVER_ERROR;
            goto error;
    }

    if ((reply_len = jerry_get_string_length(reply_payload_val))) {
        reply_buf = js_strdup(reply_payload_val);
        if (!reply_buf) {
            reply_len = 0;
            if (reply_code == COAP_CODE_CONTENT) {
                reply_code = COAP_CODE_INTERNAL_SERVER_ERROR;
            }
        }
    }

error:

    _gcoap_req.len = coap_reply_simple(_gcoap_req.pkt, reply_code, _gcoap_req.buf, _gcoap_req.len,
            COAP_FORMAT_NONE, reply_buf, reply_len);

    if (reply_buf) {
        free(reply_buf);
    }

    jerry_release_value(reply_payload_val);
    jerry_release_value(ret_val);
    jerry_release_value(payload_val);
    jerry_release_value(methods_val);
    jerry_release_value(object);

    mutex_unlock(&_gcoap_mutex);
}

static void _js_coap_handler_freecb(void *native_p)
{
    DEBUG("%s:l%u:%s()\n", __FILE__, __LINE__, __func__);

    js_coap_handler_t *js_coap_handler = (js_coap_handler_t *) native_p;

    gcoap_unregister_listener(&js_coap_handler->listener);

    free((void*)js_coap_handler->resource.path);
    free(js_coap_handler);
}

static const jerry_object_native_info_t js_coap_handler_object_native_info =
{
    .free_cb = _js_coap_handler_freecb
};

static jerry_value_t js_coap_handler_create(jerry_value_t callback, const char *path, unsigned methods)
{
    DEBUG("%s():l%u %s 0x%08x\n", __FILE__, __LINE__, __func__, methods);
    jerry_value_t object = js_object_native_create(sizeof(js_coap_handler_t), &js_coap_handler_object_native_info);
    js_coap_handler_t *js_coap_handler = js_get_object_native_pointer(object, &js_coap_handler_object_native_info);

    if (!js_coap_handler) {
        DEBUG("%s():l%u\n", __FILE__, __LINE__);
    }

    js_add_object(object, callback, "_callback");

    memset(js_coap_handler, '\0', sizeof(*js_coap_handler));

    js_coap_handler->callback = callback;

    js_coap_handler->event.callback = _js_coap_handler_event_cb;
    js_coap_handler->event.callback = _js_coap_handler_event_cb;

    js_coap_handler->resource.path = path;
    js_coap_handler->resource.methods = methods;
    js_coap_handler->resource.handler = _js_coap_resource_handler;
    js_coap_handler->resource.context = js_coap_handler;

    js_coap_handler->listener.resources = &js_coap_handler->resource;
    js_coap_handler->listener.resources_len = 1;

    js_native_ref_add(&js_coap_handler->ref, object);

    gcoap_register_listener(&js_coap_handler->listener);

    return object;
}

static JS_EXTERNAL_HANDLER(coap_register_handler)
{
    (void)func_value;
    (void)this_value;

    DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);

    if (args_cnt < 3) {
        puts("coap.register_handler(): not enough arguments");
        return 0;
    }

    if (!jerry_value_is_string(args_p[0])) {
        puts("coap.register_handler(): arg 0 not a string ");
        return 0;
    }
    if (!jerry_value_is_number(args_p[1])) {
        puts("coap.register_handler(): arg 1 not a number (expected or'ed coap methods)");
        return 0;
    }
    if (!jerry_value_is_function(args_p[2])) {
        puts("coap.register_handler(): arg 2 not a function");
        return 0;
    }

    char *path = js_strdup(args_p[0]);
    if (!path) {
        DEBUG("%s():l%u\n", __FILE__, __LINE__);
        return jerry_create_undefined();
    }

    return js_coap_handler_create(args_p[2], path, (unsigned)jerry_get_number_value(args_p[1]));
}

const js_native_method_t coap_methods[] = {
    { "register_handler", js_external_handler_coap_register_handler }
};

const unsigned coap_methods_len = sizeof(coap_methods) / sizeof(coap_methods[0]);
