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

/////////////////////////////////////////////////////////////////////////////
// -------------------- DOT LIBRARY REQUIRED ------------------------------//
// * Because these example programs can be used for both mDot and xDot     //
//     devices, the LoRa stack is not included. The libmDot library should //
//     be imported if building for mDot devices. The libxDot library       //
//     should be imported if building for xDot devices.                    //
// * https://developer.mbed.org/teams/MultiTech/code/libmDot-dev/          //
// * https://developer.mbed.org/teams/MultiTech/code/libmDot/              //
// * https://developer.mbed.org/teams/MultiTech/code/libxDot-dev/          //
// * https://developer.mbed.org/teams/MultiTech/code/libxDot/              //
/////////////////////////////////////////////////////////////////////////////


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


/////////////////////////////////////////////////////////////
// * these options must match the settings on your gateway //
// * edit their values to match your configuration         //
// * frequency sub band is only relevant for the 915 bands //
// * either the network name and passphrase can be used or //
//     the network ID (8 bytes) and KEY (16 bytes)         //
/////////////////////////////////////////////////////////////

static std::string network_name = "sad face";
static std::string network_passphrase = "happy face";
static uint8_t network_id[] = { 0x6C, 0x4E, 0xEF, 0x66, 0xF4, 0x79, 0x86, 0xA6 };
static uint8_t network_key[] = { 0x1F, 0x33, 0xA1, 0x70, 0xA5, 0xF1, 0xFD, 0xA0, 0xAB, 0x69, 0x7A, 0xAE, 0x2B, 0x95, 0x91, 0x6B };
static uint8_t frequency_sub_band = 1;
static lora::NetworkType network_type = lora::PUBLIC_LORAWAN;
static uint8_t join_delay = 5;
static uint8_t ack = 1;
static bool adr = true;

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);

#if defined(TARGET_XDOT_L151CC)
I2C i2c(I2C_SDA, I2C_SCL);
ISL29011 lux(i2c);
#else
AnalogIn lux(XBEE_AD0);
#endif


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
    update_ota_config_name_phrase(network_name, network_passphrase, frequency_sub_band, network_type, ack);
    //update_ota_config_id_key(network_id, network_key, frequency_sub_band, network_type, ack);

    // configure the Dot for class C operation
    // the Dot must also be configured on the gateway for class C
    // use the lora-query application to do this on a Conduit: http://www.multitech.net/developer/software/lora/lora-network-server/
    // to provision your Dot for class C operation with a 3rd party gateway, see the gateway or network provider documentation
    logInfo("changing network mode to class C");
    if (dot->setClass("C") != mDot::MDOT_OK) {
        logError("failed to set network mode to class C");
    }

    // enable or disable Adaptive Data Rate
    dot->setAdr(adr);

    // Configure the join delay
    dot->setJoinDelay(join_delay);

    // save changes to configuration
    logInfo("saving configuration");
    if (!dot->saveConfig()) {
        logError("failed to save configuration");
    }

    // display configuration
    display_config();

    while (true) {
        uint16_t light;
        std::vector<uint8_t> tx_data;

        // join network if not joined
        if (!dot->getNetworkJoinStatus()) {
            join_network();
        }

#if defined(TARGET_XDOT_L151CC)
        // configure the ISL29011 sensor on the xDot-DK for continuous ambient light sampling, 16 bit conversion, and maximum range
        lux.setMode(ISL29011::ALS_CONT);
        lux.setResolution(ISL29011::ADC_16BIT);
        lux.setRange(ISL29011::RNG_64000);

        // get the latest light sample and send it to the gateway
        light = lux.getData();
        tx_data.push_back((light >> 8) & 0xFF);
        tx_data.push_back(light & 0xFF);
        logInfo("light: %lu [0x%04X]", light, light);
        send_data(tx_data);

        // put the LSL29011 ambient light sensor into a low power state
        lux.setMode(ISL29011::PWR_DOWN);
#else
        // get some dummy data and send it to the gateway
        light = lux.read_u16();
        tx_data.push_back((light >> 8) & 0xFF);
        tx_data.push_back(light & 0xFF);
        logInfo("light: %lu [0x%04X]", light, light);
        send_data(tx_data);
#endif

        // the Dot can't sleep in class C mode
        // it must be waiting for data from the gateway
        // send data every 30s
        if (Fota::getInstance()->timeToStart() != 0) {
            logInfo("waiting for 30s");
            ThisThread::sleep_for(30s);
        } else {
            // Reduce uplinks during FOTA, dot cannot receive while transmitting
            // Too many lost packets will cause FOTA to fail
            logInfo("FOTA starting in %d seconds", Fota::getInstance()->timeToStart());
            logInfo("waiting for 300s");
            ThisThread::sleep_for(300s);
        }

    }

    return 0;
}

#endif

