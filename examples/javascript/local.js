/*
 * Copyright (C) 2018 Inria Emmanuel Baccelli <emmanuel.baccelli@inria.fr>
 *               2017 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */


// Memo of SAUL entries on IoT-Lab M3 open nodes
//#0	ACT_SWITCH	LED(red)
//#1	ACT_SWITCH	LED(green)
//#2	ACT_SWITCH	LED(orange)
//#3	SENSE_ACCEL	lsm303dlhc
//#4	SENSE_MAG (saul_type 0x86)	lsm303dlhc
//#5	SENSE_PRESS	lps331ap
//#6	SENSE_TEMP (saul_type 0x82)	lps331ap
//#7	SENSE_LIGHT	isl29020
//#8	SENSE_GYRO	l3g4200d

saul.get_by_name = function (name) {
    var res;
    switch (name) {
        case "led":
            res = saul._find_name("LED(red)");
            break;
        case "pressure":
            res = saul._find_name("lps331ap");
            break;
        case "temperature":
            res = saul._get_one(0x82);
            break;
        case "accelerometer":
            res = saul._find_name("lsm303dlhc");
            break;
        case "magnetometer":
            res = saul._get_one(0x86); 
            break;
        case "brightness":
            res = saul._find_name("isl29020");
            break;
        case "gyrometer":
            res = saul._find_name("l3g4200d");
            break;
    }
    saul.set_methods(res);
    return res;
}

coap._register_handler = coap.register_handler;
coap.register_handler = function(path, methods, callback) {
    var _callback = function(method) {
        var res = callback(method);

        if (res == false) {
            this.handler.unregister();
        }

        return coap.code.CHANGED;
    }
    coap._register_handler(path, methods, _callback);
}

coap.request = coap.request_sync;
