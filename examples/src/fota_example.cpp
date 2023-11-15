#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == FOTA_EXAMPLE

#if !defined(FOTA)
#define FOTA 1
#endif

#if defined(TARGET_XDOT_L151CC) && defined(FOTA)
#include "SPIFBlockDevice.h"
#include "DataFlashBlockDevice.h"
#endif

////////////////////////////////////////////////////////////////////////////
// -------------------- DEFINITIONS REQUIRED ---------------------------- //
// Add define for FOTA in mbed_app.json or on command line                //
//   Command line                                                         //
//     mbed compile -t GCC_ARM -m MTS_MDOT_F411RE -DFOTA=1                //
//   mbed_app.json                                                        //
//     {                                                                  //
//        "macros": [                                                     //
//          "FOTA"                                                        //
//        ]                                                               //
//     }                                                                  //
//                                                                        //
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
// -------------------------- XDOT EXTERNAL STORAGE --------------------- //
// An external storage device is required for FOTA on an XDot.  The       //
// storage device must meet the following criteria:                       //
// * Work with MBed OS DataFlashBlockDevice or SPIFBlockDevice classes    //
// * Maximum 4KB sector erase size                                        //
// * Maximum 512 byte page size                                           //
// * SPIF type components must support Serial Flash Discoverable          //
//   Parameters (SFDP)                                                    //
//                                                                        //
// Refer to mbed_app.json included in this project for configuration      //
// parameters requried for external storage.                              //
//                                                                        //
// Modify code below to create a BlockDevice object.                      //
////////////////////////////////////////////////////////////////////////////


mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);

#if defined(TARGET_XDOT_L151CC) && defined(FOTA)

mbed::BlockDevice* ext_bd = NULL;

mbed::BlockDevice * mdot_override_external_block_device()
{
    if (ext_bd == NULL) {
        ext_bd = new SPIFBlockDevice();
        int ret = ext_bd->init();
        if (ext_bd->init() < 0) {
            delete ext_bd;
            ext_bd = new DataFlashBlockDevice();
            ret = ext_bd->init();
            // Check for zero size because DataFlashBlockDevice doesn't
            // return an error if the chip is not present
            if ((ret < 0) || (ext_bd->size() == 0)) {
                delete ext_bd;
                ext_bd = NULL;
            }
        }

        if (ext_bd != NULL) {
            logInfo("External flash device detected, type: %s, size: 0x%08x",
                ext_bd->get_type(), (uint32_t)ext_bd->size());
        }
    }

    return ext_bd;
}
#endif


int main() {
    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    pc.baud(115200);

#if defined(TARGET_XDOT_L151CC)
    i2c.frequency(400000);
#endif

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    logInfo("----- Running FOTA example -----");

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    dot = mDot::getInstance(plan);
    assert(dot);

    logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    // Initialize FOTA singleton
    Fota::getInstance(dot);


    // start from a well-known state
    logInfo("defaulting Dot configuration");
    dot->resetConfig();
    dot->resetNetworkSession();

    // make sure library logging is turned on
    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

    // attach the custom events handler
    dot->setEvents(&events);

    // Enable FOTA for multicast support
    Fota::getInstance(dot);

    // update configuration if necessary
    if (dot->getJoinMode() != mDot::OTA) {
        logInfo("changing network join mode to OTA");
        if (dot->setJoinMode(mDot::OTA) != mDot::MDOT_OK) {
            logError("failed to set network join mode to OTA");
        }
    }
    // in OTA and AUTO_OTA join modes, the credentials can be passed to the library as a name and passphrase or an ID and KEY
    // only one method or the other should be used!
    // network ID = crc64(network name)
    // network KEY = cmac(network passphrase)
#if defined(DERIVE_FROM_TEXT)
        update_ota_config_name_phrase(cfg::network_name, cfg::network_passphrase, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#else
        update_ota_config_id_key(cfg::network_id, cfg::network_key, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#endif

    // configure the Dot for class C operation
    // the Dot must also be configured on the gateway for class C
    // use the lora-query application to do this on a Conduit: http://www.multitech.net/developer/software/lora/lora-network-server/
    // to provision your Dot for class C operation with a 3rd party gateway, see the gateway or network provider documentation
    logInfo("changing network mode to class C");
    if (dot->setClass("C") != mDot::MDOT_OK) {
        logError("failed to set network mode to class C");
    }

    // enable or disable Adaptive Data Rate
    dot->setAdr(cfg::adr);

    // Configure the join delay
    dot->setJoinDelay(cfg::join_delay);

    // save changes to configuration
    logInfo("saving configuration");
    if (!dot->saveConfig()) {
        logError("failed to save configuration");
    }

    // display configuration
    display_config();

    while (true) {
        static uint8_t payload_size_sent;

        // join network if not joined
        if (!dot->getNetworkJoinStatus()) {
            join_network();
        }

        // If the channel plan has duty cycle restrictions, wait may be required.
        thread_wait_for_channel();
        
        // Don't perform any extra sends during fota.
        send(payload_size_sent);
        // Since downlinks can come at anytime in class C mode, handle them in RadioEvents.h.

        // The Dot can't sleep in class C mode. It must be ready for data from the gateway.
        // Send data every 30s.
        if (Fota::getInstance()->timeToStart() != 0) {
            logInfo("waiting for 30s");
            ThisThread::sleep_for(30s);
        } else {
            // Reduce uplinks during FOTA, dot cannot receive while transmitting.
            // Too many missed downlink packets will cause FOTA to fail.
            logInfo("FOTA starting in %d seconds", Fota::getInstance()->timeToStart());
            logInfo("waiting for 300s");
            ThisThread::sleep_for(300s);
        }

    }

    return 0;
}

#endif

