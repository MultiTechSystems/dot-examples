#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == MANUAL_EXAMPLE

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

    logInfo("----- Running manual example -----");

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    dot = mDot::getInstance(plan);
    assert(dot);

    // attach the custom events handler
    dot->setEvents(&events);

    if (!dot->getStandbyFlag()) {
        logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

        // start from a well-known state
        logInfo("defaulting Dot configuration");
        dot->resetConfig();
        dot->resetNetworkSession();

        // make sure library logging is turned on
        dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

        // update configuration if necessary
        if (dot->getJoinMode() != mDot::MANUAL) {
            logInfo("changing network join mode to MANUAL");
            if (dot->setJoinMode(mDot::MANUAL) != mDot::MDOT_OK) {
                logError("failed to set network join mode to MANUAL");
            }
        }
        // In MANUAL join mode, there is no join request/response transaction. As long as the Dot is configured
        // correctly and provisioned correctly on the gateway, it should be able to communicate.
        // network address - 4 bytes (00000001 - FFFFFFFE)
        // network session key - 16 bytes
        // data session key - 16 bytes
        // to provision your Dot with a Conduit gateway, follow the following steps
        //   * ssh into the Conduit
        //   * provision the Dot using the lora-query application: http://www.multitech.net/developer/software/lora/lora-network-server/
        //      lora-query -a 01020304 A 0102030401020304 <your Dot's device ID> 01020304010203040102030401020304 01020304010203040102030401020304
        //   * if you change the network address, network session key, or data session key, make sure you update them on the gateway
        // to provision your Dot with a 3rd party gateway, see the gateway or network provider documentation
        update_manual_config(cfg::network_address, cfg::network_session_key, cfg::data_session_key, cfg::frequency_sub_band, cfg::network_type, cfg::ack);

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
        logInfo("restoring network session from NVM");
        dot->restoreNetworkSession();
    }

    while (true) {
        // Defensive programming in case the gateway/network server continuously gives a reason to send.
        const uint8_t max_consecutive_sends = 4;
        static uint8_t consecutive_sends = max_consecutive_sends;
        static uint8_t payload_size_sent;

        // In MANUAL join mode, there is no join request/response transaction. As long as the Dot is configured
        // correctly and provisioned correctly on the gateway, it should be able to communicate.

        // If the channel plan has duty cycle restrictions, wait may be required.
        dot_wait_for_channel();

        if(send(payload_size_sent) == mDot::MDOT_OK) {
            // In class A mode, downlinks only occur following an uplink. So process downlinks after a successful send.
            if (events.PacketReceived && (events.RxPort == (dot->getAppPort()))) {
                std::vector<uint8_t> rx_data;
                if (dot->recv(rx_data) == mDot::MDOT_OK) {
                    logInfo("Downlink data (port %d) %s", dot->getAppPort(),mts::Text::bin2hexString(rx_data.data(), rx_data.size()).c_str());
                }
            }
            // Data pending is set for the following reasons.
            // 1. There are downlinks queued up for this endpoint. This reason is cleared on send and updated on reception
            //    of a downlink. So, a missed downlink results in no data pending for this reason.
            // 2. There are MAC command answers pending.
            // 3. An Ack has been requested of this endpoint.
            if ((dot->getDataPending() || (payload_size_sent == 0)) && consecutive_sends > 1) {
                // Don't sleep and send again. 
                consecutive_sends--;
            } else {
                consecutive_sends = max_consecutive_sends;
                dot_sleep();
            }
        } else { // Send failed. Don't drain battery by repeatedly sending.
            dot_sleep();
        }
    }

    return 0;
}

#endif
