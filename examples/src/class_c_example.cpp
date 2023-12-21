#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == CLASS_C_EXAMPLE

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

    logInfo("----- Running class C example -----");

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
        // Defensive programming in case the gateway/network server continuously gives a reason to send.
        static const uint8_t max_consecutive_sends = 4;
        static uint8_t consecutive_sends = max_consecutive_sends;
        static uint8_t payload_size_sent;

        // Join network if join status indicates not joined. If link check threshold is not enabled, another method
        // should be used to ensure connectivity and trigger joins. This could be based on not seeing a downlink for 
        // an extended period of time.
        if (!dot->getNetworkJoinStatus()) {
            join_network();
        }

        // If the channel plan has duty cycle restrictions, wait may be required.
        thread_wait_for_channel();

        if((send(payload_size_sent) == mDot::MDOT_OK) && (payload_size_sent == 0)) {
            // Sent empty payload intending to clear MAC commands.
            // Warning: Payload always 0 if payload is larger than data rate allows.
            // Since downlinks can come at anytime in class C mode, handle them in RadioEvents.h.
        }

        consecutive_sends--;
        // Optional reasons to send again right away.
        // 1. There are MAC command answers pending.
        // 2. An Ack has been requested of this endpoint.
        // 3. Sent an empty payload to clear MAC commands. dot->hasMacCommands is not true now but that's because an 
        //    empty packet was sent making room for the actual payload to be sent.
        uint8_t sleep_s = 30;
        logInfo("Wait up to %d seconds before sending again", sleep_s);
        while ((!dot->hasMacCommands() && !dot->getAckRequested() && !(payload_size_sent == 0)) || consecutive_sends <= 1) {
            // The Dot can't sleep in class C mode. Here only this thread is sleeping. During this time, it will still
            // receive downlinks which are handled in RadioEvents.h.
            // Sends data every 30s.
            consecutive_sends = max_consecutive_sends;
            ThisThread::sleep_for(1s);
            if (--sleep_s == 0)
                break;
        }
        logInfo("Waited %d seconds", 30-sleep_s);
        if (sleep_s > 0) {
            if(dot->hasMacCommands())
                logInfo("Respond with MAC answers");
            if(dot->getAckRequested())
                logInfo("Ack has been requested");
            if(payload_size_sent == 0)
                logInfo("Sent an empty payload to clear MAC commands");
            if(consecutive_sends <= 1)
                logInfo("Reached consecutive send limit of %d without sleeping", max_consecutive_sends);
        }
    }

    return 0;
}

#endif

