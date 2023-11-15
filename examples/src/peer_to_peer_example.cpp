#include "dot_util.h"
#include "RadioEvent.h"

#if ACTIVE_EXAMPLE == PEER_TO_PEER_EXAMPLE

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);

int main() {
    // Custom event handler for automatically displaying RX data
    RadioEvent events;
    uint32_t tx_frequency;
    uint8_t tx_datarate;
    uint8_t tx_power;
    uint8_t frequency_band;

    pc.baud(115200);

#if defined(TARGET_XDOT_L151CC)
    i2c.frequency(400000);
#endif

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    logInfo("----- Running peer to peer example -----");

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    dot = mDot::getInstance(plan);
    assert(dot);

    logInfo("mbed-os library version: %d.%d.%d", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    // start from a well-known state
    logInfo("defaulting Dot configuration");
    dot->resetConfig();

    // make sure library logging is turned on
    dot->setLogLevel(mts::MTSLog::INFO_LEVEL);

    // attach the custom events handler
    dot->setEvents(&events);

    // Enable FOTA for multicast support
    Fota::getInstance(dot);

    // update configuration if necessary
    if (dot->getJoinMode() != mDot::PEER_TO_PEER) {
        logInfo("changing network join mode to PEER_TO_PEER");
        if (dot->setJoinMode(mDot::PEER_TO_PEER) != mDot::MDOT_OK) {
            logError("failed to set network join mode to PEER_TO_PEER");
        }
    }
    frequency_band = dot->getFrequencyBand();
    switch (frequency_band) {
        case lora::ChannelPlan::EU868_OLD:
        case lora::ChannelPlan::EU868:
            // 250kHz channels achieve higher throughput
            // DR_6 : SF7 @ 250kHz
            // DR_0 - DR_5 (125kHz channels) available but much slower
            tx_frequency = 869850000;
            tx_datarate = lora::DR_6;
            // the 869850000 frequency is 100% duty cycle if the total power is under 7 dBm - tx power 4 + antenna gain 3 = 7
            tx_power = 4;
            break;

        case lora::ChannelPlan::US915_OLD:
        case lora::ChannelPlan::US915:
        case lora::ChannelPlan::AU915_OLD:
        case lora::ChannelPlan::AU915:
            // 500kHz channels achieve highest throughput
            // DR_8 : SF12 @ 500kHz
            // DR_9 : SF11 @ 500kHz
            // DR_10 : SF10 @ 500kHz
            // DR_11 : SF9 @ 500kHz
            // DR_12 : SF8 @ 500kHz
            // DR_13 : SF7 @ 500kHz
            // DR_0 - DR_3 (125kHz channels) available but much slower
            tx_frequency = 915500000;
            tx_datarate = lora::DR_13;
            // 915 bands have no duty cycle restrictions, set tx power to max
            tx_power = 20;
            break;

        case lora::ChannelPlan::AS923:
        case lora::ChannelPlan::AS923_JAPAN:
            // 250kHz channels achieve higher throughput
            // DR_6 : SF7 @ 250kHz
            // DR_0 - DR_5 (125kHz channels) available but much slower
            tx_frequency = 924800000;
            tx_datarate = lora::DR_6;
            tx_power = 16;
            break;

        case lora::ChannelPlan::KR920:
            // DR_5 : SF7 @ 125kHz
            tx_frequency = 922700000;
            tx_datarate = lora::DR_5;
            tx_power = 14;
            break;

        default:
            while (true) {
                logFatal("no known channel plan in use - extra configuration is needed!");
                ThisThread::sleep_for(5s);
            }
            break;
    }
    // in PEER_TO_PEER mode there is no join request/response transaction
    // as long as both Dots are configured correctly, they should be able to communicate
    update_peer_to_peer_config(cfg::network_address, cfg::network_session_key, cfg::data_session_key, tx_frequency, tx_datarate, tx_power);

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

        send(payload_size_sent);
        // Since downlinks can come at anytime, handle them in RadioEvents.h.

        // The Dot can't sleep in PEER_TO_PEER mode. It must be waiting for data from the other Dot.
        // Send data every 5 seconds
        logInfo("waiting for 5s");
        ThisThread::sleep_for(5s);
    }

    return 0;
}

#endif
