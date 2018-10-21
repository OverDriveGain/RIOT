       var callback = function () {
            return false;
        }
        coap.register_handler("cancel alarm", 0x01, callback);

       var callback2 = function () {
           coap.register_handler("cancel alarm", 0x01, callback);
           return callback2();
       }
       callback2();
