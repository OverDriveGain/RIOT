#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitarithm.h"
#include "mutex.h"
#include "event.h"
#include "js.h"
#include "net/gcoap.h"
#include "net/sock/util.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#ifndef JS_COAP_INBOUND_MAXLEN
#define JS_COAP_INBOUND_MAXLEN 64
#endif

static mutex_t _gcoap_mutex = MUTEX_INIT;

typedef struct {
    event_t event;
    coap_resource_t resource;
    gcoap_listener_t listener;
    jerry_value_t callback;
    js_native_ref_t ref;
} js_coap_handler_t;

static struct {
    coap_pkt_t *pkt;
    uint8_t *buf;
    size_t len;
} _gcoap_req;

static ssize_t _js_coap_resource_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, void *context)
{
    js_coap_handler_t *js_coap_handler = context;

    DEBUG("%s:l%u:%s()\n", __FILE__, __LINE__, __func__);
    mutex_lock(&_gcoap_mutex);
    DEBUG("%s:l%u:%s() delegating...\n", __FILE__, __LINE__, __func__);

    _gcoap_req.pkt = pkt;
    _gcoap_req.buf = buf;
    _gcoap_req.len = len;

    if (js_coap_handler->event.list_node.next) {
        DEBUG("%s:l%u:%s() event already queued\n", __FILE__, __LINE__, __func__);
        return 0;
    }
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
    if (jerry_value_is_error(object)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        goto error;
    }

    methods_val = jerry_create_number(coap_method2flag(coap_get_code_detail(_gcoap_req.pkt)));
    if (jerry_value_is_error(methods_val)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        goto error;
    }

    if (_gcoap_req.pkt->payload_len) {
        if (_gcoap_req.pkt->payload_len > JS_COAP_INBOUND_MAXLEN) {
            DEBUG("%s():l%u %s inbound coap payload too large\n", __FILE__, __LINE__, __func__);
            goto error;
        }

        payload_val = jerry_create_string_sz((jerry_char_t *)_gcoap_req.pkt->payload, _gcoap_req.pkt->payload_len);
        if (jerry_value_is_error(payload_val)) {
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
            goto error;
        }
        js_add_object(object, payload_val, "payload");
    }

    /* add coap resource handler to request object */
    js_add_object(object, js_coap_handler->ref.object, "handler");

    jerry_value_t args[] = { methods_val, payload_val };
    ret_val = jerry_call_function(js_coap_handler->callback, object, args, payload_val ? 2 : 1);

    if (jerry_value_is_error(ret_val)) {
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
        case COAP_CODE_CONTENT:
            break;
        default:
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
            reply_code = COAP_CODE_INTERNAL_SERVER_ERROR;
            goto error;
    }

    if ((reply_len = jerry_get_string_length(reply_payload_val))) {
        reply_buf = js_strdup(reply_payload_val);
        if (!reply_buf) {
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
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

    if (js_coap_handler->listener.resources_len) {
        gcoap_unregister_listener(&js_coap_handler->listener);
    }

    free((void*)js_coap_handler->resource.path);
    free(js_coap_handler);
}

static const jerry_object_native_info_t js_coap_handler_object_native_info =
{
    .free_cb = _js_coap_handler_freecb
};

static JS_EXTERNAL_HANDLER(coap_handler_unregister)
{
    (void)func_value;
    (void)args_p;
    (void)args_cnt;

    DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);

    js_coap_handler_t *js_coap_handler = js_get_object_native_pointer(this_value, &js_coap_handler_object_native_info);
    if (!js_coap_handler) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        return jerry_create_undefined();
    }

    gcoap_unregister_listener(&js_coap_handler->listener);
    js_coap_handler->listener.resources_len = 0;
    js_native_ref_rem(&js_coap_handler->ref);
    return 0;
}

static jerry_value_t js_coap_handler_create(jerry_value_t callback, const char *path, unsigned methods)
{
    DEBUG("%s():l%u %s 0x%08x\n", __FILE__, __LINE__, __func__, methods);
    jerry_value_t object = js_object_native_create(sizeof(js_coap_handler_t), &js_coap_handler_object_native_info);
    js_coap_handler_t *js_coap_handler = js_get_object_native_pointer(object, &js_coap_handler_object_native_info);

    if (!js_coap_handler) {
        DEBUG("%s():l%u\n", __FILE__, __LINE__);
    }

    js_add_object(object, callback, "_callback");
    js_add_external_handler(object, "unregister", js_external_handler_coap_handler_unregister);

    memset(js_coap_handler, '\0', sizeof(*js_coap_handler));

    js_coap_handler->callback = callback;

    js_coap_handler->event.handler = _js_coap_handler_event_cb;

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

static mutex_t _gcoap_request_mutex;
static unsigned _req_state;
static coap_pkt_t* _pdu;

static void _gcoap_req_resp_handler(unsigned req_state, coap_pkt_t* pdu, sock_udp_ep_t *remote)
{
    (void)remote;
    DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
    if (req_state != GCOAP_MEMO_WAIT) {
        _req_state = req_state;
        _pdu = pdu;
        mutex_unlock(&_gcoap_request_mutex);
    }
}

static JS_EXTERNAL_HANDLER(coap_request)
{
    (void)func_value;
    (void)this_value;

    DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);

    if (args_cnt < 2) {
        puts("coap.request_sync(): not enough arguments");
        return 0;
    }

    if (!jerry_value_is_string(args_p[0])) {
        puts("coap.request_sync(): arg 0 not a string ");
        return 0;
    }
    if (!jerry_value_is_number(args_p[1])) {
        puts("coap.request_sync(): arg 1 not a number (expected or'ed coap methods)");
        return 0;
    }


    coap_pkt_t pkt;
    uint8_t buf[256];
    unsigned urllen = jerry_string_to_char_buffer(args_p[0], buf, sizeof(buf));
    buf[urllen] = '\0';

    char hostport[SOCK_HOSTPORT_MAXLEN];
    char urlpath[SOCK_URLPATH_MAXLEN];

    if(sock_urlsplit((char*)buf, hostport, urlpath)) {
        DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
        return 0;
    }

    DEBUG("%s: sending to hostport=\"%s\" urlpath=\"%s\"\n", __func__, hostport, urlpath);

    unsigned method = jerry_get_number_value(args_p[1]);
    method = bitarithm_lsb(method) + 1;

    unsigned len = 0;
    gcoap_req_init(&pkt, buf, sizeof(buf), method, urlpath);
    if (args_cnt > 2) {
        if (!jerry_value_is_string(args_p[2])) {
            puts("coap.request_sync(): arg 2 not a string");
            return 0;
        }
        len = jerry_string_to_char_buffer(args_p[2], pkt.payload, pkt.payload_len);
        if (!len) {
            DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
            return 0;
        }
    }
    len = gcoap_finish(&pkt, len, len ? COAP_FORMAT_TEXT : COAP_FORMAT_NONE);

    sock_udp_ep_t remote;
    if (sock_udp_str2ep(&remote, hostport)) {
        return 0;
    }

    if (!remote.port) {
        remote.port = 5683;
    }

    mutex_init(&_gcoap_request_mutex);
    mutex_lock(&_gcoap_request_mutex);
    DEBUG("%s():l%u %s\n", __FILE__, __LINE__, __func__);
    size_t res = gcoap_req_send2(buf, len, &remote, _gcoap_req_resp_handler);
    DEBUG("%s():l%u %s res=%i\n", __FILE__, __LINE__, __func__, (int)res);
    mutex_lock(&_gcoap_request_mutex);
    DEBUG("%s():l%u %s %u %p\n", __FILE__, __LINE__, __func__, _req_state, (void*)_pdu);
    if (_req_state == GCOAP_MEMO_RESP) {
        assert(_pdu);
        if (!_pdu->payload_len) {
            return jerry_create_boolean(1);
        }
        return jerry_create_string_sz(_pdu->payload, _pdu->payload_len);
    }
    else {
        return jerry_create_boolean(0);
    }
}

const js_native_method_t coap_methods[] = {
    { "register_handler", js_external_handler_coap_register_handler },
    { "request_sync", js_external_handler_coap_request }
};

const unsigned coap_methods_len = sizeof(coap_methods) / sizeof(coap_methods[0]);
