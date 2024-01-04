#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == FOTA_EXAMPLE

#if !defined(FOTA)
#define FOTA 1
#endif

#if defined(TARGET_XDOT_L151CC) || defined(TARGET_XDOT_MAX32670) && defined(FOTA)
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

#if defined(TARGET_XDOT_L151CC) || defined(TARGET_XDOT_MAX32670) && defined(FOTA)

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

/** Provide allowable sleep interval for FOTA.
*
* @param sleep_interval is the requested sleep interval.
*/
void get_fota_sleep_interval(uint32_t &sleep_interval) {
    if (Fota::getInstance()->ready()) {
        printf("fota time to start = %us\r\n", Fota::getInstance()->timeToStart());
        if (sleep_interval > Fota::getInstance()->timeToStart()) {
            sleep_interval = Fota::getInstance()->timeToStart();
        } else {
            sleep_interval = 0;
        }
    // These are both false after FOTA but before CRC. Need to send CRC before sleep.
    } else if (!Fota::getInstance()->idle() && !Fota::getInstance()->ready()) {
        sleep_interval = 0;
    }
}

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

    // attach the custom events handler
    dot->setEvents(&events);

    // Enable FOTA for multicast support
    Fota::getInstance(dot);

    if (!dot->getStandbyFlag() && !dot->getPreserveSession()) {
        logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
        
        // start from a well-known state
        logInfo("defaulting Dot configuration");
        dot->resetConfig();
        dot->resetNetworkSession();

        // make sure library logging is turned on
        dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

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

        // configure network link checks
        // network link checks are a good alternative to requiring the gateway to ACK every packet and should allow a single gateway to handle more Dots
        // check the link every count packets
        // declare the Dot disconnected after threshold failed link checks
        // for count = 3 and threshold = 5, the Dot will ask for a link check response every 5 packets and will consider the connection lost if it fails to receive 3 responses in a row
        update_network_link_check_config(3, 5);
        
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
    } else {
        // restore the saved session if the dot woke from deepsleep mode
        // useful to use with deepsleep because session info is otherwise lost when the dot enters deepsleep
#if defined(TARGET_XDOT_MAX32670)
        // Saving and restoring session around deep sleep is automatic for xDot-AD.
#else        
        logInfo("restoring network session from NVM");
        dot->restoreNetworkSession();
#endif
    }

    while (true) {
        static uint8_t payload_size_sent;
        // The Dot starts out in class A and can only receive a FOTA session request on an uplink. Make sure this
        // interval is short enough to provide at least three opportunities for downlinks.
        static uint32_t sleep_interval_s = 10;

        // Join network if join status indicates not joined. If link check threshold is not enabled, another method
        // should be used to ensure connectivity and trigger joins. This could be based on not seeing a downlink for 
        // an extended period of time.
        if (!dot->getNetworkJoinStatus()) {
            join_network();
        }

        // If the channel plan has duty cycle restrictions, wait may be required.
        thread_wait_for_channel();
        
        // Don't perform any extra sends during fota.
        send(payload_size_sent);
        // Since downlinks can come at anytime in class C mode, handle them in RadioEvents.h.

        if (!dot->getNetworkJoinStatus()) {
            // skip any sleep and join.
        } else {
            get_fota_sleep_interval(sleep_interval_s);
            if (sleep_interval_s) {
                // Dot can sleep at this time as FOTA is not active.
                sleep_wake_rtc_only(sleep_interval_s, true);
            } else {
                // FOTA is occurring. Only use thread sleep. Must be able to receive FOTA packets at any time. Limit
                // uplink frequency as too many will cause excessive missed FOTA packets.
                ThisThread::sleep_for(300s);
            }
        }
    }

    return 0;
}

#endif

