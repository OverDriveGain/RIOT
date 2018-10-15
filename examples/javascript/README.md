### About

This example shows how to push Javascript logic, sent via CoAP, to be executed in a tiny software container hosted on an IoT device running RIOT. 

If you need to reference this work in an academic context, please cite: *E. Baccelli, J. Doerr, S. Kikuchi, FJA Padilla, K. Schleiser, I. Thomas, ''Scripting Over-The-Air: Towards Containers on Low-end Devices in the Internet of Things'', in IEEE PerCom, 2018 (to appear)*.

While the below instructions show how to run the example on hardware available remotely on the open-access testbed [IoT-Lab](https://www.iot-lab.info), the same example can run on your own RIOT-supported hardware. For instance see this other [guide](https://github.com/emmanuelsearch/RIOT/blob/js-coap-samr21/examples/script-coap/README.md) which uses a RasPi as border router and a SAMR21 as IoT device.


### Preliminaries
The below instructions assume that you have: 

1. the setup to compile RIOT (if needed refer to the beginner [tutorial](https://github.com/RIOT-OS/Tutorials/blob/master/README.md))
2. an account on the (free) open-access testbed [IoT-Lab](https://www.iot-lab.info)
1. a CoAP client, e.g. Firefox browser extension [Copper](https://addons.mozilla.org/en-US/firefox/addon/copper-270430/)
1. IPv6 connectivity e.g. using a free tunnel provider such as [Hurricane Electric](https://tunnelbroker.net)
1. (if not done already git clone this [repo](https://github.com/emmanuelsearch/RIOT.git) and check out the branch *riot.js.demo.iotlab*)

### Setup

![RIOT Javascript Container Example Setup](https://github.com/emmanuelsearch/RIOT/blob/riot.js.demo.iotlab/examples/javascript/setup-iotlab.png)

1. In RIOT/examples/javascript compile this example with the command 
```
BOARD=iotlab-m3 make
```

1. The resulting binary is then *riot_javascript.elf* located in RIOT/examples/javascript/bin/iotlab-m3/

1. Reserve & program two nodes on IoT-lab following their [Public IPv6 tutorial](https://www.iot-lab.info/tutorials/riot-public-ipv66lowpan-network-with-m3-nodes/). This step provides you with an IoT device (the first node) connected wirelessly to the global IPv6 network via the second node acting as border router (using 6LoWPAN and IEEE 802.15.4 radio).

1. Go back to step 7 onwards in the [Public IPv6 tutorial](https://www.iot-lab.info/tutorials/riot-public-ipv66lowpan-network-with-m3-nodes/) and flash *riot\_javascript.elf* instead of the initial gnrc\_networking.elf (you may skip this step if you directly flash riot\_javascript.elf instead of gnrc\_networking.elf in the previous step)

2. On the RIOT shell running on the IoT device, you should see its global IPv6 address which you need to send CoAP requests to in the next section.


### How to run

In your CoAP client, type or copy/paste your Javascript as payload of a PUT request to the CoAP resource */riot/script* on the IoT-lab M3 node (using the IPv6 address of the node obtained as indicated above at setup).

As an example of script, you may first try to push and execute the simple program:
```
print('Hello RIOT container!');
```

In your terminal window with the RIOT shell running on the IoT device, you should see the script being correctly received, and then executed.

You may repeat this procedure at will, pushing arbitrary scripts via CoAP. For instance, see below for a more elaborate script example.

### Sample Javascript with sensor/actuator access

Using the above procedure, you can try to push the below script, for instance:

```
// Get handles on sensors and actuators

this.accelerometer = saul.get_by_name("accelerometer");
this.led = saul.get_by_name("led");
this.pressure = saul.get_by_name("pressure");
this.temperature = saul.get_by_name("temperature");
this.accelerometer = saul.get_by_name("accelerometer");
this.magnetometer = saul.get_by_name("magnetometer");
this.brightness = saul.get_by_name("brightness");
this.gyrometer = saul.get_by_name("gyrometer");

// Print out some measurements

print("Pressure = " + this.pressure.read() + " (mBar)");
print("Brightness = " + this.brightness.read() + " (cd)");
print("Temperature = " + (this.temperature.read()/100) + " (Celsius)");
print("Initial LED status = " + this.led.read());
this.led.write(0);
print("Modified LED status = " + this.led.read());
this.led.write(1);
```

