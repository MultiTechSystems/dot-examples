# dot-examples
Example programs for MultiTech Dot devices demonstrating how to use the Dot devices and the Dot libraries for LoRa communication.

## Instructions for building Dot-Examples

1. Import dot-examples  
$ mbed import http://os.mbed.com/teams/MultiTech/code/Dot-Examples/

2. Change directory to the Dot-Examples/examples and import the dot library stack  
Choose either the stable or dev library for your dot device  
e.g. to get the latest development library for the xDot  
$ mbed add http://os.mbed.com/teams/MultiTech/code/libxDot-dev-mbed5/

3. Update mbed-os revision to match that of the dot library you just imported.  
This information can be found in the library's commit history.
e.g.  
$ cd Dot-Examples/mbed-os  
$ mbed update mbed-os-5.11.1

4. Modify the Dot-Examples/examples/example_config.h to select the channel plan and which example to build  
By default, the OTA example is selected to build with the US channel plan

5. Once the example is selected, modify the example source file to match the configuration of your gateway.
Make sure the network_name, network_passphrase, frequency_sub_band (US), public_network, and join_delay settings match that of your gateway

## Example Programs Description
This application contains multiple example programs. Each example demonstrates a different way to configure and use a Dot. A short summary of each example is provided below. Common code used by multiple examples is in the dot_utils.cpp file.

All examples print logging, including RX data, on the USB debug port at 115200 baud. Each example defaults the Dot's configuration and saves the new configuration to NVM. 

### OTA Example
This example demonstrates configuring the Dot for OTA join mode and entering sleep or deepsleep mode between transactions with the gateway. If deepsleep mode is used, the session is saved and restored so that a rejoin is not necessary after waking up even though RAM contents have been lost. ACKs are disabled, but network link checks are configured - if enough link checks are missed, the Dot will no longer be considered joined to the network and will attempt to rejoin before transmitting more data. 

### AUTO_OTA Example
This example demonstrates configuring the Dot for AUTO_OTA join mode and entering sleep or deepsleep mode between transactions with the gateway. AUTO_OTA join mode automatically saves and restores the session when deepsleep mode is used, so the manual saving and restoring of the session is not necessary. ACKs are disabled, but network link checks are configured - if enough link checks are missed, the Dot will no longer be considered joined to the network and will attempt to rejoin before transmitting more data. 

### Manual Example
This example demonstrates configuring the Dot for MANUAL join mode and entering sleep or deepsleep mode between transactions with the gateway. The Dot must be provisioned on the gateway before its packets will be accepted! Follow these steps to provision the Dot on a Conduit gateway:

* ssh into the conduit
* use the lora-query application to provision the Dot on the gateway
lora-query -a 01020304 A 0102030401020304 <your Dot's device ID> 01020304010203040102030401020304 01020304010203040102030401020304
* if any of the credentials change on the Dot side, they must be updated on the gateway side as well
To provision a Dot on a third-party gateway, see the gateway or network provider documentation. 

### Class C Example
This example demonstrates configuring the Dot for OTA join mode and communicating with the gateway using class C mode. In class C mode the gateway can send a packet to the Dot at any time, so it must be listening whenever it is not transmitting. This means that the Dot cannot enter sleep or deepsleep mode. The gateway will not immediately send packets to the Dot (outside the receive windows following a transmission from the Dot) until it is informed that the Dot is operating in class C mode. The lora-query application can be used to configure a Conduit gateway to communicate with a Dot in class C mode. For information on how to inform a third-party gateway that a Dot is operating in class C mode, see the gateway or network provider documentation. 

### Class B Example
This example demonstrates how to configure the dot for an OTA join, how to acquire a lock on a GPS synchronized beacon, and then to subsequently enter class B mode of operation.  After a successful join, the device will request to the dot-library to switch to class B. When this happens, the library will send an uplink to the network server (hence we must be joined first before entering this mode) requesting the GPS time to calculate when the next beacon is expected. Once this time elapses, the dot will open an rx window to demodulate the broadcasted beacon and fire an mDotEvent::BeaconRx event upon successful reception. After the beacon is received, the example sends an uplink which will have the class B bit in the packet's frame control set to indicate to the network server that downlinks may now be scheduled on ping slots. The lora-query application can be used to configure a Conduit gateway to communicate with a Dot in class B mode. For information on how to inform a third-party gateway that a Dot is operating in class B mode, see the gateway or network provider documentation.

### FOTA Example
Full FOTA support is only available with mDot, xDot does not have the required external flash. xDot can use the FOTA example to dynamically join a multicast session only. After joining the multicast session the received Fragmentation packets could be handed to a host MCU for processing and at completion the firmware can be loaded into the xDot using the bootloader and y-modem. See [xDot Developer Guide](https://www.multitech.com/brands/multiconnect-xdot).

This example demonstrates how to incorporate over-the-air updates to an application. The example uses a Class C application. Class A or B functionality could also be used. The device will automatically enter into Class C operation for the FOTA operation, Class B would be disabled during the FOTA transfer.

Add the following code to allow Fota to use the Dot instance
>examples/src/fota_example.cpp
```c
    // Initialize FOTA singleton
    Fota::getInstance(dot);
```

Add fragmentation handling the the PacketRx event
>examples/inc/RadioEvent.h
```c
    virtual void PacketRx(uint8_t port, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr, lora::DownlinkControl ctrl, uint8_t slot, uint8_t retries, uint32_t address, bool dupRx) {
        mDotEvent::PacketRx(port, payload, size, rssi, snr, ctrl, slot, retries, address, dupRx);

#if ACTIVE_EXAMPLE == FOTA_EXAMPLE
        if(port == 200 || port == 201 || port == 202) {
            Fota::getInstance()->processCmd(payload, port, size);
        }
#endif
    }
```

The FOTA implementation has a few differences from the [LoRaWAN FOTA Protocol](https://lora-alliance.org/resource-hub/lorawan-fragmented-data-block-transport-specification-v100)
* Fragmentation Indexing starts at 0
* McKEKey is 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00
* Start Time is a count-down in seconds to start of session


### Peer to Peer Example
This example demonstrates configuring Dots for peer to peer communication without a gateway. It should be compiled and run on two Dots. Peer to peer communication uses LoRa modulation but uses a single higher throughput (usually 500kHz or 250kHz) datarate. It is similar to class C operation - when a Dot isn't transmitting, it's listening for packets from the other Dot. Both Dots must be configured exactly the same for peer to peer communication to be successful. 


## Choosing An Example Program and Channel Plan
Only the active example is compiled. The active example can be updated by changing the **ACTIVE_EXAMPLE** definition in the examples/example_config.h file.

By default the OTA_EXAMPLE will be compiled and the US915 channel plan will be used.
>example_config.h

```c
#ifndef __EXAMPLE__CONFIG_H__
#define __EXAMPLE__CONFIG_H__
 
#define OTA_EXAMPLE              1  // see ota_example.cpp
#define AUTO_OTA_EXAMPLE         2  // see auto_ota_example.cpp
#define MANUAL_EXAMPLE           3  // see manual_example.cpp
#define PEER_TO_PEER_EXAMPLE     4  // see peer_to_peer_example.cpp
#define CLASS_C_EXAMPLE          5  // see class_c_example.cpp
#define CLASS_B_EXAMPLE          6  // see class_b_example.cpp
#define FOTA_EXAMPLE             7  // see fota_example.cpp
 
// the active example is the one that will be compiled
#if !defined(ACTIVE_EXAMPLE)
#define ACTIVE_EXAMPLE  OTA_EXAMPLE
#endif
 
// the active channel plan is the one that will be compiled
// options are :
//      CP_US915
//      CP_AU915
//      CP_EU868
//      CP_KR920
//      CP_AS923
//      CP_AS923_JAPAN
#if !defined(CHANNEL_PLAN)
#define CHANNEL_PLAN CP_US915
#endif
 
#endif
```

Compile the AUTO_OTA_EXAMPLE and use the EU868 channel plan instead.
```c
#ifndef __EXAMPLE__CONFIG_H__
#define __EXAMPLE__CONFIG_H__
 
#define OTA_EXAMPLE              1  // see ota_example.cpp
#define AUTO_OTA_EXAMPLE         2  // see auto_ota_example.cpp
#define MANUAL_EXAMPLE           3  // see manual_example.cpp
#define PEER_TO_PEER_EXAMPLE     4  // see peer_to_peer_example.cpp
#define CLASS_C_EXAMPLE          5  // see class_c_example.cpp
 
// the active example is the one that will be compiled
#if !defined(ACTIVE_EXAMPLE)
#define ACTIVE_EXAMPLE  AUTO_OTA_EXAMPLE
#endif
 
// the active channel plan is the one that will be compiled
// options are :
//      CP_US915
//      CP_AU915
//      CP_EU868
//      CP_KR920
//      CP_AS923
//      CP_AS923_JAPAN
#if !defined(CHANNEL_PLAN)
#define CHANNEL_PLAN CP_EU868
#endif
 
#endif
```

## Dot Libraries
Stable and development libraries are available for both mDot and xDot platforms. The library chosen must match the target platform. Compiling for the mDot platform with the xDot library or vice versa will not succeed.

### mDot Library
#### Development library for mDot

Bleeding edge development version of the mDot library for mbed 5. This version of the library is not guaranteed to be stable or well tested and should not be used in production or deployment scenarios.  

mbed add [https://github.com/MultiTechSystems/libmDot-dev-mbed5](https://github.com/MultiTechSystems/libmDot-dev-mbed5)

#### Stable library for mDot

Stable version of the mDot library for mbed 5. This version of the library is suitable for deployment scenarios. See lastest commit message for version of mbed-os library that has been tested against.  

mbed add [https://github.com/MultiTechSystems/libmDot-mbed5](https://github.com/MultiTechSystems/libmDot-mbed5)
  
### xDot Library
#### Development library for xDot
Bleeding edge development version of the xDot library for mbed 5. This version of the library is not guaranteed to be stable or well tested and should not be used in production or deployment scenarios.  

mbed add [https://github.com/MultiTechSystems/libxDot-dev-mbed5](https://github.com/MultiTechSystems/libxDot-dev-mbed5)

#### Stable library for xDot

Stable version of the xDot library for mbed 5. This version of the library is suitable for deployment scenarios.  

mbed add [https://github.com/MultiTechSystems/libxDot-mbed5](https://github.com/MultiTechSystems/libxDot-mbed5)
