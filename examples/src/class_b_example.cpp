#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == CLASS_B_EXAMPLE

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

/////////////////////////////////////////////////////////////
// * these options must match the settings on your gateway //
// * edit their values to match your configuration         //
// * frequency sub band is only relevant for the 915 bands //
// * either the network name and passphrase can be used or //
//     the network ID (8 bytes) and KEY (16 bytes)         //
/////////////////////////////////////////////////////////////
static std::string network_name = "MultiTech";
static std::string network_passphrase = "MultiTech";
static uint8_t network_id[] = { 0x6C, 0x4E, 0xEF, 0x66, 0xF4, 0x79, 0x86, 0xA6 };
static uint8_t network_key[] = { 0x1F, 0x33, 0xA1, 0x70, 0xA5, 0xF1, 0xFD, 0xA0, 0xAB, 0x69, 0x7A, 0xAE, 0x2B, 0x95, 0x91, 0x6B };
static uint8_t frequency_sub_band = 1;
static lora::NetworkType public_network = lora::PUBLIC_LORAWAN;
static uint8_t join_delay = 5;
static uint8_t ack = 3;
static bool adr = true;

// Number of ping slots to open per beacon interval - see mDot.h
static uint8_t ping_periodicity = 4;

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);

#if defined(TARGET_XDOT_L151CC)
I2C i2c(I2C_SDA, I2C_SCL);
ISL29011 lux(i2c);
#elif defined(TARGET_XDOT_MAX32670)
// no analog available
#else
AnalogIn lux(XBEE_AD0);
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
    update_ota_config_name_phrase(network_name, network_passphrase, frequency_sub_band, public_network, ack);
    //update_ota_config_id_key(network_id, network_key, frequency_sub_band, public_network, ack);

    // enable or disable Adaptive Data Rate
    dot->setAdr(adr);

    // Configure the join delay
    dot->setJoinDelay(join_delay);

    // Configure the class B ping periodicity
    dot->setPingPeriodicity(ping_periodicity);

    // save changes to configuration
    logInfo("saving configuration");
    if (!dot->saveConfig()) {
        logError("failed to save configuration");
    }

    // display configuration
    display_config();

    // join the network - must do this before attempting to switch to class B
    join_network();

    // configure the Dot for class B operation
    // the Dot must also be configured on the gateway for class B
    // use the lora-query application to do this on a Conduit: http://www.multitech.net/developer/software/lora/lora-network-server/
    // to provision your Dot for class B operation with a 3rd party gateway, see the gateway or network provider documentation
    // Note: we won't actually switch to class B until we receive a beacon (mDotEvent::BeaconRx fires)
    logInfo("changing network mode to class B");

    if (dot->setClass("B") != mDot::MDOT_OK) {
        logError("Failed to set network mode to class B");
        logInfo("Reset the MCU to try again");
        return 0;
    }

    // Start a timer to check the beacon was acquired
    LowPowerTimer bcn_timer;
    bcn_timer.start();

    while (true) {
        std::vector<uint8_t> tx_data;
        static bool send_uplink = true;

        // Check if we locked the beacon yet and send an uplink to notify the network server
        // To receive data from the gateway in class B ping slots, we must have received a beacon
        // already, and sent one uplink to signal to the network server that we are in class B mode
        if (events.BeaconLocked && send_uplink) {
            logInfo("Acquired a beacon lock");

            // Add a random delay before trying the uplink to avoid collisions w/ other motes
            srand(dot->getRadioRandom());
            uint32_t rand_delay = rand() % 5000;
            logInfo("Applying a random delay of %d ms before class notification uplink", rand_delay);
            ThisThread::sleep_for(std::chrono::milliseconds(rand_delay));

            // Ensure the link is idle before trying to transmit
            while (!dot->getIsIdle()) {
                ThisThread::sleep_for(10ms);
            }

            if (send_data(tx_data) != mDot::MDOT_OK) {
                logError("Failed to inform the network server we are in class B");
                logInfo("Reset the MCU to try again");
                return 0;
            }

            logInfo("Enqueued packets may now be scheduled on class B ping slots");
            send_uplink = false;
            bcn_timer.stop();
        } else if (!events.BeaconLocked) {
            logInfo("Waiting to receive a beacon..");

            if (bcn_timer.read() > lora::DEFAULT_BEACON_PERIOD) {
                if (dot->setClass("B") != mDot::MDOT_OK) {
                    logError("Failed to set network mode to class B");
                    logInfo("Reset the MCU to try again");
                    return 0;
                }

                bcn_timer.reset();
            }
        }
        ThisThread::sleep_for(10s);
    }

    return 0;
}

#endif
