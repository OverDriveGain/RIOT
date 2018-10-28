var callback = function () {
     coap.register_handler("cancel alarm", 0x01, callback);
     return callback();
}
callback();
