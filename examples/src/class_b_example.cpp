#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == CLASS_B_EXAMPLE

// Number of ping slots to open per beacon interval - see mDot.h
static uint8_t ping_periodicity = 4;

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);

int main() {
    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    pc.baud(115200);

#if defined(TARGET_XDOT_L151CC)
    i2c.frequency(400000);
#endif

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    logInfo("----- Running class B example -----");

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
#if defined(DERIVE_FROM_TEXT)
    update_ota_config_name_phrase(cfg::network_name, cfg::network_passphrase, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#else
    update_ota_config_id_key(cfg::network_id, cfg::network_key, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#endif

    // enable or disable Adaptive Data Rate
    dot->setAdr(cfg::adr);

    // Configure the join delay
    dot->setJoinDelay(cfg::join_delay);

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

            if (send_data() != mDot::MDOT_OK) {
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
