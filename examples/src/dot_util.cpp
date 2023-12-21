#include "dot_util.h"

#if defined(TARGET_XDOT_L151CC)
#include "xdot_low_power.h"
#elif defined(TARGET_XDOT_MAX32670)
#include "LowPower.h"
#endif

void sleep_wake_rtc_only(uint32_t sleep_s, bool deepsleep);
void sleep_wake_interrupt_only(bool deepsleep);
void sleep_wake_rtc_or_interrupt(uint32_t sleep_s, bool deepsleep);

#if defined(TARGET_MTS_MDOT_F411RE)
uint32_t portA[6];
uint32_t portB[6];
uint32_t portC[6];
uint32_t portD[6];
uint32_t portH[6];
#endif


lora::ChannelPlan* create_channel_plan() {
    lora::ChannelPlan* plan;

#if CHANNEL_PLAN == CP_US915
    plan = new lora::ChannelPlan_US915();
#elif CHANNEL_PLAN == CP_AU915
    plan = new lora::ChannelPlan_AU915();
#elif CHANNEL_PLAN == CP_EU868
    plan = new lora::ChannelPlan_EU868();
#elif CHANNEL_PLAN == CP_KR920
    plan = new lora::ChannelPlan_KR920();
#elif CHANNEL_PLAN == CP_IN865
    plan = new lora::ChannelPlan_IN865();
#elif CHANNEL_PLAN == CP_AS923
    plan = new lora::ChannelPlan_AS923();
#elif CHANNEL_PLAN == CP_AS923_2
    plan = new lora::ChannelPlan_AS923();
#elif CHANNEL_PLAN == CP_AS923_3
    plan = new lora::ChannelPlan_AS923();
#elif CHANNEL_PLAN == CP_AS923_4
    plan = new lora::ChannelPlan_AS923();
#elif CHANNEL_PLAN == CP_AS923_JAPAN
    plan = new lora::ChannelPlan_AS923_Japan();
#elif CHANNEL_PLAN == CP_AS923_JAPAN1
    plan = new lora::ChannelPlan_AS923_Japan1();
#elif CHANNEL_PLAN == CP_AS923_JAPAN2
    plan = new lora::ChannelPlan_AS923_Japan2();
#elif CHANNEL_PLAN == CP_RU864
    plan = new lora::ChannelPlan_RU864();
#elif CHANNEL_PLAN == CP_GLOBAL
    #if GLOBAL_PLAN == CP_US915
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::US915);
    #elif GLOBAL_PLAN == CP_EU868
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::EU868);
    #elif GLOBAL_PLAN == CP_AU915
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AU915);
    #elif GLOBAL_PLAN == CP_AS923
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AS923);
    #elif GLOBAL_PLAN == CP_AS923_2
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AS923_2);
    #elif GLOBAL_PLAN == CP_AS923_3
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AS923_3);
    #elif GLOBAL_PLAN == CP_AS923_4
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AS923_4);
    #elif GLOBAL_PLAN == CP_AS923_JAPAN
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::AS923_JAPAN);
    #elif GLOBAL_PLAN == CP_KR920
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::KR920);
    #elif GLOBAL_PLAN == CP_IN865
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::IN865);
    #elif GLOBAL_PLAN == CP_RU864
    plan = new lora::ChannelPlan_GLOBAL(lora::ChannelPlan::RU864);
    #endif
#endif

    return plan;
}


void display_config() {
    // display configuration and library version information
    logInfo("=====================");
    logInfo("general configuration");
    logInfo("=====================");
    logInfo("version ------------------ %s", dot->getId().c_str());
    logInfo("device ID/EUI ------------ %s", mts::Text::bin2hexString(dot->getDeviceId()).c_str());
    logInfo("default channel plan ----- %s", mDot::FrequencyBandStr(dot->getDefaultFrequencyBand()).c_str());
    logInfo("current channel plan ----- %s", mDot::FrequencyBandStr(dot->getFrequencyBand()).c_str());
    if (lora::ChannelPlan::IsPlanFixed(dot->getFrequencyBand())) {
        logInfo("frequency sub band ------- %u", dot->getFrequencySubBand());
    }

    std::string network_mode_str("Undefined");
    uint8_t network_mode = dot->getPublicNetwork();
    if (network_mode == lora::PRIVATE_MTS)
        network_mode_str = "Private MTS";
    else if (network_mode == lora::PUBLIC_LORAWAN)
        network_mode_str = "Public LoRaWAN";
    else if (network_mode == lora::PRIVATE_LORAWAN)
        network_mode_str = "Private LoRaWAN";
    logInfo("public network ----------- %s", network_mode_str.c_str());
    logInfo("join delay  -------------- %d", dot->getJoinDelay());
    logInfo("join nonce validation ---- %s", dot->getJoinNonceValidation() ? "enabled" : "disabled");
    logInfo("=========================");
    logInfo("credentials configuration");
    logInfo("=========================");
    logInfo("device class ------------- %s", dot->getClass().c_str());
    logInfo("network join mode -------- %s", mDot::JoinModeStr(dot->getJoinMode()).c_str());
    if (dot->getJoinMode() == mDot::MANUAL || dot->getJoinMode() == mDot::PEER_TO_PEER) {
	logInfo("network address ---------- %s", mts::Text::bin2hexString(dot->getNetworkAddress()).c_str());
	logInfo("network session key------- %s", mts::Text::bin2hexString(dot->getNetworkSessionKey()).c_str());
	logInfo("data session key---------- %s", mts::Text::bin2hexString(dot->getDataSessionKey()).c_str());
    } else {
#if defined(DERIVE_FROM_TEXT)
	logInfo("network name ------------- %s", dot->getNetworkName().c_str());
	logInfo("network phrase ----------- %s", dot->getNetworkPassphrase().c_str());
#else
	logInfo("network EUI -------------- %s", mts::Text::bin2hexString(dot->getNetworkId()).c_str());
	logInfo("network KEY -------------- %s", mts::Text::bin2hexString(dot->getNetworkKey()).c_str());
#endif
    }
    logInfo("========================");
    logInfo("communication parameters");
    logInfo("========================");
    if (dot->getJoinMode() == mDot::PEER_TO_PEER) {
	logInfo("TX frequency ------------- %lu", dot->getTxFrequency());
    } else {
	logInfo("acks --------------------- %s, %u attempts", dot->getAck() > 0 ? "enabled" : "disabled", dot->getAck());
    }
    logInfo("link check count --------- %d", dot->getLinkCheckCount());
    logInfo("link check threshold ----- %d", dot->getLinkCheckThreshold());
    logInfo("adaptive data rate ------- %s", dot->getAdr() == 0 ? "disabled" : "enabled");
    if (dot->getAdr()) {
    logInfo("--- ADRAckLimit ---------- %d", dot->getAdrAckLimit());
    logInfo("--- ADRAckDelay ---------- %d", dot->getAdrAckDelay());
    logInfo("--- ADR auto increment --- %s", dot->getDisableIncrementDR() == 0 ? "disabled" : "enabled");
    }
    logInfo("TX datarate -------------- %s", mDot::DataRateStr(dot->getTxDataRate()).c_str());
    logInfo("TX power ----------------- %lu dBm", dot->getTxPower());
    logInfo("antenna gain ------------- %u dBm", dot->getAntennaGain());
    logInfo("LBT ---------------------- %s", dot->getLbtTimeUs() ? "on" : "off");
    if (dot->getLbtTimeUs()) {
	logInfo("LBT time ----------------- %lu us", dot->getLbtTimeUs());
	logInfo("LBT threshold ------------ %d dBm", dot->getLbtThreshold());
    }
}

void update_ota_config_name_phrase(std::string network_name, std::string network_passphrase, uint8_t frequency_sub_band, lora::NetworkType network_type, uint8_t ack) {
    std::string current_network_name = dot->getNetworkName();
    std::string current_network_passphrase = dot->getNetworkPassphrase();
    uint8_t current_frequency_sub_band = dot->getFrequencySubBand();
    uint8_t current_network_type = dot->getPublicNetwork();
    uint8_t current_ack = dot->getAck();

    if (current_network_name != network_name) {
        logInfo("changing network name from \"%s\" to \"%s\"", current_network_name.c_str(), network_name.c_str());
        if (dot->setNetworkName(network_name) != mDot::MDOT_OK) {
            logError("failed to set network name to \"%s\"", network_name.c_str());
        }
    }

    if (current_network_passphrase != network_passphrase) {
        logInfo("changing network passphrase from \"%s\" to \"%s\"", current_network_passphrase.c_str(), network_passphrase.c_str());
        if (dot->setNetworkPassphrase(network_passphrase) != mDot::MDOT_OK) {
            logError("failed to set network passphrase to \"%s\"", network_passphrase.c_str());
        }
    }

    if (lora::ChannelPlan::IsPlanFixed(dot->getFrequencyBand())) {
	if (current_frequency_sub_band != frequency_sub_band) {
	    logInfo("changing frequency sub band from %u to %u", current_frequency_sub_band, frequency_sub_band);
	    if (dot->setFrequencySubBand(frequency_sub_band) != mDot::MDOT_OK) {
		logError("failed to set frequency sub band to %u", frequency_sub_band);
	    }
	}
    }

    if (current_network_type != network_type) {
        if (dot->setPublicNetwork(network_type) != mDot::MDOT_OK) {
            logError("failed to set network type");
        }
    }

    if (current_ack != ack) {
        logInfo("changing acks from %u to %u", current_ack, ack);
        if (dot->setAck(ack) != mDot::MDOT_OK) {
            logError("failed to set acks to %u", ack);
        }
    }
}

void update_ota_config_id_key(const uint8_t *network_id, const uint8_t *network_key, uint8_t frequency_sub_band, lora::NetworkType network_type, uint8_t ack) {
    std::vector<uint8_t> current_network_id = dot->getNetworkId();
    std::vector<uint8_t> current_network_key = dot->getNetworkKey();
    uint8_t current_frequency_sub_band = dot->getFrequencySubBand();
    uint8_t current_network_type = dot->getPublicNetwork();
    uint8_t current_ack = dot->getAck();

    std::vector<uint8_t> network_id_vector(network_id, network_id + 8);
    std::vector<uint8_t> network_key_vector(network_key, network_key + 16);

    if (current_network_id != network_id_vector) {
        logInfo("changing network ID from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_id).c_str(), mts::Text::bin2hexString(network_id_vector).c_str());
        if (dot->setNetworkId(network_id_vector) != mDot::MDOT_OK) {
            logError("failed to set network ID to \"%s\"", mts::Text::bin2hexString(network_id_vector).c_str());
        }
    }

    if (current_network_key != network_key_vector) {
        logInfo("changing network KEY from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_key).c_str(), mts::Text::bin2hexString(network_key_vector).c_str());
        if (dot->setNetworkKey(network_key_vector) != mDot::MDOT_OK) {
            logError("failed to set network KEY to \"%s\"", mts::Text::bin2hexString(network_key_vector).c_str());
        }
    }

    if (lora::ChannelPlan::IsPlanFixed(dot->getFrequencyBand())) {
	if (current_frequency_sub_band != frequency_sub_band) {
	    logInfo("changing frequency sub band from %u to %u", current_frequency_sub_band, frequency_sub_band);
	    if (dot->setFrequencySubBand(frequency_sub_band) != mDot::MDOT_OK) {
		logError("failed to set frequency sub band to %u", frequency_sub_band);
	    }
	}
    }

    if (current_network_type != network_type) {
        if (dot->setPublicNetwork(network_type) != mDot::MDOT_OK) {
            logError("failed to set network type");
        }
    }

    if (current_ack != ack) {
        logInfo("changing acks from %u to %u", current_ack, ack);
        if (dot->setAck(ack) != mDot::MDOT_OK) {
            logError("failed to set acks to %u", ack);
        }
    }
}

void update_manual_config(const uint8_t *network_address, const uint8_t *network_session_key, const uint8_t *data_session_key, uint8_t frequency_sub_band, lora::NetworkType network_type, uint8_t ack) {
    std::vector<uint8_t> current_network_address = dot->getNetworkAddress();
    std::vector<uint8_t> current_network_session_key = dot->getNetworkSessionKey();
    std::vector<uint8_t> current_data_session_key = dot->getDataSessionKey();
    uint8_t current_frequency_sub_band = dot->getFrequencySubBand();
    uint8_t current_network_type = dot->getPublicNetwork();
    uint8_t current_ack = dot->getAck();

    std::vector<uint8_t> network_address_vector(network_address, network_address + 4);
    std::vector<uint8_t> network_session_key_vector(network_session_key, network_session_key + 16);
    std::vector<uint8_t> data_session_key_vector(data_session_key, data_session_key + 16);

    if (current_network_address != network_address_vector) {
        logInfo("changing network address from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_address).c_str(), mts::Text::bin2hexString(network_address_vector).c_str());
        if (dot->setNetworkAddress(network_address_vector) != mDot::MDOT_OK) {
            logError("failed to set network address to \"%s\"", mts::Text::bin2hexString(network_address_vector).c_str());
        }
    }

    if (current_network_session_key != network_session_key_vector) {
        logInfo("changing network session key from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_session_key).c_str(), mts::Text::bin2hexString(network_session_key_vector).c_str());
        if (dot->setNetworkSessionKey(network_session_key_vector) != mDot::MDOT_OK) {
            logError("failed to set network session key to \"%s\"", mts::Text::bin2hexString(network_session_key_vector).c_str());
        }
    }

    if (current_data_session_key != data_session_key_vector) {
        logInfo("changing data session key from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_data_session_key).c_str(), mts::Text::bin2hexString(data_session_key_vector).c_str());
        if (dot->setDataSessionKey(data_session_key_vector) != mDot::MDOT_OK) {
            logError("failed to set data session key to \"%s\"", mts::Text::bin2hexString(data_session_key_vector).c_str());
        }
    }

    if (current_frequency_sub_band != frequency_sub_band) {
        logInfo("changing frequency sub band from %u to %u", current_frequency_sub_band, frequency_sub_band);
        if (dot->setFrequencySubBand(frequency_sub_band) != mDot::MDOT_OK) {
            logError("failed to set frequency sub band to %u", frequency_sub_band);
        }
    }

    if (current_network_type != network_type) {
        if (dot->setPublicNetwork(network_type) != mDot::MDOT_OK) {
            logError("failed to set network type");
        }
    }

    if (current_ack != ack) {
        logInfo("changing acks from %u to %u", current_ack, ack);
        if (dot->setAck(ack) != mDot::MDOT_OK) {
            logError("failed to set acks to %u", ack);
        }
    }
}

void update_peer_to_peer_config(const uint8_t *network_address, const uint8_t *network_session_key, const uint8_t *data_session_key, uint32_t tx_frequency, uint8_t tx_datarate, uint8_t tx_power) {
    std::vector<uint8_t> current_network_address = dot->getNetworkAddress();
    std::vector<uint8_t> current_network_session_key = dot->getNetworkSessionKey();
    std::vector<uint8_t> current_data_session_key = dot->getDataSessionKey();
    uint32_t current_tx_frequency = dot->getTxFrequency();
    uint8_t current_tx_datarate = dot->getTxDataRate();
    uint8_t current_tx_power = dot->getTxPower();

    std::vector<uint8_t> network_address_vector(network_address, network_address + 4);
    std::vector<uint8_t> network_session_key_vector(network_session_key, network_session_key + 16);
    std::vector<uint8_t> data_session_key_vector(data_session_key, data_session_key + 16);

    if (current_network_address != network_address_vector) {
        logInfo("changing network address from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_address).c_str(), mts::Text::bin2hexString(network_address_vector).c_str());
        if (dot->setNetworkAddress(network_address_vector) != mDot::MDOT_OK) {
            logError("failed to set network address to \"%s\"", mts::Text::bin2hexString(network_address_vector).c_str());
        }
    }

    if (current_network_session_key != network_session_key_vector) {
        logInfo("changing network session key from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_network_session_key).c_str(), mts::Text::bin2hexString(network_session_key_vector).c_str());
        if (dot->setNetworkSessionKey(network_session_key_vector) != mDot::MDOT_OK) {
            logError("failed to set network session key to \"%s\"", mts::Text::bin2hexString(network_session_key_vector).c_str());
        }
    }

    if (current_data_session_key != data_session_key_vector) {
        logInfo("changing data session key from \"%s\" to \"%s\"", mts::Text::bin2hexString(current_data_session_key).c_str(), mts::Text::bin2hexString(data_session_key_vector).c_str());
        if (dot->setDataSessionKey(data_session_key_vector) != mDot::MDOT_OK) {
            logError("failed to set data session key to \"%s\"", mts::Text::bin2hexString(data_session_key_vector).c_str());
        }
    }

    if (current_tx_frequency != tx_frequency) {
        logInfo("changing TX frequency from %lu to %lu", current_tx_frequency, tx_frequency);
        if (dot->setTxFrequency(tx_frequency) != mDot::MDOT_OK) {
            logError("failed to set TX frequency to %lu", tx_frequency);
        }
    }

    if (current_tx_datarate != tx_datarate) {
        logInfo("changing TX datarate from %u to %u", current_tx_datarate, tx_datarate);
        if (dot->setTxDataRate(tx_datarate) != mDot::MDOT_OK) {
            logError("failed to set TX datarate to %u", tx_datarate);
        }
    }

    if (current_tx_power != tx_power) {
        logInfo("changing TX power from %u to %u", current_tx_power, tx_power);
        if (dot->setTxPower(tx_power) != mDot::MDOT_OK) {
            logError("failed to set TX power to %u", tx_power);
        }
    }
}

void update_network_link_check_config(uint8_t link_check_count, uint8_t link_check_threshold) {
    uint8_t current_link_check_count = dot->getLinkCheckCount();
    uint8_t current_link_check_threshold = dot->getLinkCheckThreshold();

    if (current_link_check_count != link_check_count) {
        logInfo("changing link check count from %u to %u", current_link_check_count, link_check_count);
        if (dot->setLinkCheckCount(link_check_count) != mDot::MDOT_OK) {
            logError("failed to set link check count to %u", link_check_count);
        }
    }

    if (current_link_check_threshold != link_check_threshold) {
        logInfo("changing link check threshold from %u to %u", current_link_check_threshold, link_check_threshold);
        if (dot->setLinkCheckThreshold(link_check_threshold) != mDot::MDOT_OK) {
            logError("failed to set link check threshold to %u", link_check_threshold);
        }
    }
}

void join_network() {
    int8_t j_attempts = 2;
    int32_t ret;

    for (int8_t i = 0; i < j_attempts; i++) {
        // This is needed for all channel plans. Even if the channel plan does not have duty cycle restrictions,
        // join will have some amount of random back off for the rare case where many endpoints
        // power up at the same time. Staggering joins will help ensure successful joins.
        dot_wait_for_channel();
        logInfo("Join try %d of %d", i+1, j_attempts);
        ret = dot->joinNetwork();
        if (ret == mDot::MDOT_OK) {
            return;
        }
    }

    // If link check threshold is disabled, it can make sense to restore the old session on join failure as it
    // could still be valid.
    if (dot->getLinkCheckThreshold() == 0) {
        dot->restoreNetworkSession();
    }
    else {
        // Do not restore the session if link check threshold is enabled. This can set the join status back to joined
        // when it is legitimately lost.
    }

    // On join failure check the dev EUI as it can cause join failures.
    std::vector<uint8_t> devEUI = dot->getDeviceId();
    bool validEUI = false;
    for (uint8_t i : devEUI) {
        if (i != 0 && i != 0xff) {
            validEUI = true;
            break;
        }
    }
    if(!validEUI) {
        logError("********** Dev EUI erased/invalid! Must be programmed to join! **********");
        // Read the dev EUI from the dot's label and reprogram it.
        /*std::vector<uint8_t> newDevEUI {0x00, 0x08, 0x00, 0x00, 0x04, 0x04, 0x2f, 0x82};
        if (dot->setDeviceId(newDevEUI)!= mDot::MDOT_OK) {
            logError("failed to set dev EUI");
        }*/
    }
}

void dot_sleep(){
    if(cfg::wake_mode == cfg::interval)
        sleep_wake_rtc_only(cfg::sleep_seconds, cfg::deep_sleep);
    else if(cfg::wake_mode == cfg::interrupt)
        sleep_wake_interrupt_only(cfg::deep_sleep);
    else if(cfg::wake_mode == cfg::interval_or_interrupt)
        sleep_wake_rtc_or_interrupt(cfg::sleep_seconds, cfg::deep_sleep);
    else
        logError("Invalid wake mode %d", cfg::wake_mode);
}

void sleep_wake_rtc_only(uint32_t sleep_s, bool deepsleep) {
#if defined(TARGET_XDOT_MAX32670) || (ACTIVE_EXAMPLE == AUTO_OTA_EXAMPLE)
    // xDot-AD automatically saves when going into deepsleep and restores on wakeup.
    // All dots automatically save when going into deepsleep and restore on wakeup with auto OTA mode is set.
#else
    // In cases where session information would be lost over deepsleep save it.
    // Save is not necessary if going into sleep mode since RAM is retained.
    if (deepsleep) {
        logInfo("saving network session to NVM");
        dot->saveNetworkSession();
    }
#endif
    logInfo("%ssleeping %lus", deepsleep ? "deep" : "", sleep_s);
    logInfo("application will %s after waking up", deepsleep ? "execute from beginning" : "resume");

    // SLEEP MODE
    // Lowest current consumption in sleep mode can only be achieved by properly configuring all IOs. The library handles
    // all internal IOs automatically, but the external IOs are the application's responsibility. Certain IOs may
    // require internal pullup or pulldown resistors because leaving them floating would cause extra current consumption.
    // for xDot: UART_*, I2C_*, SPI_*, GPIO*, WAKE
    // for mDot: XBEE_*, USBTX, USBRX, PB_0, PB_1
    // steps are:
    //   * save IO configuration
    //   * configure IOs to reduce current consumption
    //   * sleep
    //   * restore IO configuration
    // DEEP SLEEP MODE
    // mDot: stop mode is used and both internal and external pins are automatically configured for low power by the dot
    // library code. Use sleep not deep sleep or external pull resistors if if external circuitry needs a specific state
    // for lowest power consumption.
    // xDot_L151CC: standby mode is used. So most pins are high impedance. See the reference manual for details. Use sleep
    // not deep sleep or external pull resistors if if external circuitry needs a specific state for lowest power consumption.
    // xDot_MAX32670: back up mode is used. So, the library code only configures internal pins leaving the external
    // pins to be configured by the user.
    if (! deepsleep) {
        // save the GPIO state.
        sleep_save_io();

#if defined (TARGET_XDOT_L151CC) || defined(TARGET_MTS_MDOT_F411RE)
        // configure GPIOs for lowest current if not deepsleep.
        sleep_configure_io();
    }
#elif defined (TARGET_XDOT_MAX32670)
    }
    // configure GPIOs for lowest current regardless of sleep mode.
    sleep_configure_io();
#endif

    dot->sleep(sleep_s, mDot::RTC_ALARM, deepsleep);

    if (! deepsleep) {
        // restore the GPIO state.
        sleep_restore_io();
    }
}

void sleep_wake_interrupt_only(bool deepsleep) {
#if defined(TARGET_XDOT_MAX32670) || (ACTIVE_EXAMPLE == AUTO_OTA_EXAMPLE)
    // xDot-AD automatically saves session when going into deepsleep and restores it on wakeup.
    // All dots automatically save session when going into deepsleep and restore it on wakeup when auto OTA mode is set.
#else
    // In cases where session information would be lost over deepsleep save it.
    // Save is not necessary if going into sleep mode since RAM is retained.
    if (deepsleep) {
        logInfo("saving network session to NVM");
        dot->saveNetworkSession();
    }
#endif
    dot->setWakePinMode(cfg::wake_pin_mode);
    dot->setWakePinTrigger(cfg::wake_pin_trigger);
    dot->setWakePin(dot->pinNum2Name(cfg::wake_pin));
#if defined (TARGET_XDOT_L151CC)
    logInfo("%ssleeping until interrupt on %s pin", deepsleep ? "deep" : "", deepsleep ? "WAKE" : mDot::pinName2Str(dot->getWakePin()).c_str());
#elif defined (TARGET_MTS_MDOT_F411RE)
    logInfo("%ssleeping until interrupt on %s pin", deepsleep ? "deep" : "", deepsleep ? "DIO7" : mDot::pinName2Str(dot->getWakePin()).c_str());
#elif defined(TARGET_XDOT_MAX32670)
    logInfo("%ssleeping until interrupt on %s pin", deepsleep ? "deep" : "", mDot::pinName2Str(dot->getWakePin()).c_str());
#endif

    logInfo("application will %s after waking up", deepsleep ? "execute from beginning" : "resume");

    // SLEEP MODE
    // Lowest current consumption in sleep mode can only be achieved by properly configuring all IOs. The library handles
    // all internal IOs automatically, but the external IOs are the application's responsibility. Certain IOs may
    // require internal pullup or pulldown resistors because leaving them floating would cause extra current consumption.
    // for xDot: UART_*, I2C_*, SPI_*, GPIO*, WAKE
    // for mDot: XBEE_*, USBTX, USBRX, PB_0, PB_1
    // steps are:
    //   * save IO configuration
    //   * configure IOs to reduce current consumption
    //   * sleep
    //   * restore IO configuration
    // DEEP SLEEP MODE
    // mDot: stop mode is used and both internal and external pins are automatically configured for low power by the dot
    // library code. Use sleep not deep sleep or external pull resistors if if external circuitry needs a specific state
    // for lowest power consumption.
    // xDot_L151CC: standby mode is used. So most pins are high impedance. See the reference manual for details. Use sleep
    // not deep sleep or external pull resistors if if external circuitry needs a specific state for lowest power consumption.
    // xDot_MAX32670: back up mode is used. So, the library code only configures internal pins leaving the external
    // pins to be configured by the user.
    if (! deepsleep) {
        // save the GPIO state.
        sleep_save_io();

#if defined (TARGET_XDOT_L151CC) || defined(TARGET_MTS_MDOT_F411RE)
        // configure GPIOs for lowest current if not deepsleep.
        sleep_configure_io();
    }
#elif defined (TARGET_XDOT_MAX32670)
    }
    // configure GPIOs for lowest current regardless of sleep mode.
    sleep_configure_io();
#endif

    // go to sleep/deepsleep and wake on rising edge of configured wake pin (only the WAKE pin in deepsleep)
    // since we're not waking on the RTC alarm, the interval is ignored
    dot->sleep(0, mDot::INTERRUPT, deepsleep);

    if (! deepsleep) {
        // restore the GPIO state.
        sleep_restore_io();
    }
}

void sleep_wake_rtc_or_interrupt(uint32_t sleep_s, bool deepsleep) {
#if defined(TARGET_XDOT_MAX32670) || (ACTIVE_EXAMPLE == AUTO_OTA_EXAMPLE)
    // xDot-AD automatically saves when going into deepsleep and restores on wakeup.
    // All dots automatically save when going into deepsleep and restore on wakeup with auto OTA mode is set.
#else
    // In cases where session information would be lost over deepsleep save it.
    // Save is not necessary if going into sleep mode since RAM is retained.
    if (deepsleep) {
        logInfo("saving network session to NVM");
        dot->saveNetworkSession();
    }
#endif
    dot->setWakePinMode(cfg::wake_pin_mode);
    dot->setWakePinTrigger(cfg::wake_pin_trigger);
    dot->setWakePin(dot->pinNum2Name(cfg::wake_pin));
#if defined (TARGET_XDOT_L151CC)
    logInfo("%ssleeping until interrupt on %s pin or %d seconds", deepsleep ? "deep" : "", deepsleep ? "WAKE" : mDot::pinName2Str(dot->getWakePin()).c_str(), sleep_s);
#elif defined (TARGET_MTS_MDOT_F411RE)
    logInfo("%ssleeping until interrupt on %s pin or %d seconds", deepsleep ? "deep" : "", deepsleep ? "DIO7" : mDot::pinName2Str(dot->getWakePin()).c_str(), sleep_s);
#elif defined(TARGET_XDOT_MAX32670)
    logInfo("%ssleeping until interrupt on %s pin or %d seconds", deepsleep ? "deep" : "", mDot::pinName2Str(dot->getWakePin()).c_str(), sleep_s);
#endif

    logInfo("application will %s after waking up", deepsleep ? "execute from beginning" : "resume");

    // SLEEP MODE
    // Lowest current consumption in sleep mode can only be achieved by properly configuring all IOs. The library handles
    // all internal IOs automatically, but the external IOs are the application's responsibility. Certain IOs may
    // require internal pullup or pulldown resistors because leaving them floating would cause extra current consumption.
    // for xDot: UART_*, I2C_*, SPI_*, GPIO*, WAKE
    // for mDot: XBEE_*, USBTX, USBRX, PB_0, PB_1
    // steps are:
    //   * save IO configuration
    //   * configure IOs to reduce current consumption
    //   * sleep
    //   * restore IO configuration
    // DEEP SLEEP MODE
    // mDot: stop mode is used and both internal and external pins are automatically configured for low power by the dot
    // library code. Use sleep not deep sleep or external pull resistors if if external circuitry needs a specific state
    // for lowest power consumption.
    // xDot_L151CC: standby mode is used. So most pins are high impedance. See the reference manual for details. Use sleep
    // not deep sleep or external pull resistors if if external circuitry needs a specific state for lowest power consumption.
    // xDot_MAX32670: back up mode is used. So, the library code only configures internal pins leaving the external
    // pins to be configured by the user.
    if (! deepsleep) {
        // save the GPIO state.
        sleep_save_io();

#if defined (TARGET_XDOT_L151CC) || defined(TARGET_MTS_MDOT_F411RE)
        // configure GPIOs for lowest current if not deepsleep.
        sleep_configure_io();
    }
#elif defined (TARGET_XDOT_MAX32670)
    }
    // configure GPIOs for lowest current regardless of sleep mode.
    sleep_configure_io();
#endif

    // go to sleep/deepsleep and wake using the RTC alarm after delay_s seconds or rising edge of configured wake pin (only the WAKE pin in deepsleep)
    // whichever comes first will wake the xDot
    dot->sleep(sleep_s, mDot::RTC_ALARM_OR_INTERRUPT, deepsleep);

    if (! deepsleep) {
        // restore the GPIO state.
        sleep_restore_io();
    }
}

void sleep_save_io() {
#if defined(TARGET_XDOT_L151CC)
	xdot_save_gpio_state();
#elif defined(TARGET_XDOT_MAX32670)
    // saved by sleep
#else
	portA[0] = GPIOA->MODER;
	portA[1] = GPIOA->OTYPER;
	portA[2] = GPIOA->OSPEEDR;
	portA[3] = GPIOA->PUPDR;
	portA[4] = GPIOA->AFR[0];
	portA[5] = GPIOA->AFR[1];

	portB[0] = GPIOB->MODER;
	portB[1] = GPIOB->OTYPER;
	portB[2] = GPIOB->OSPEEDR;
	portB[3] = GPIOB->PUPDR;
	portB[4] = GPIOB->AFR[0];
	portB[5] = GPIOB->AFR[1];

	portC[0] = GPIOC->MODER;
	portC[1] = GPIOC->OTYPER;
	portC[2] = GPIOC->OSPEEDR;
	portC[3] = GPIOC->PUPDR;
	portC[4] = GPIOC->AFR[0];
	portC[5] = GPIOC->AFR[1];

	portD[0] = GPIOD->MODER;
	portD[1] = GPIOD->OTYPER;
	portD[2] = GPIOD->OSPEEDR;
	portD[3] = GPIOD->PUPDR;
	portD[4] = GPIOD->AFR[0];
	portD[5] = GPIOD->AFR[1];

	portH[0] = GPIOH->MODER;
	portH[1] = GPIOH->OTYPER;
	portH[2] = GPIOH->OSPEEDR;
	portH[3] = GPIOH->PUPDR;
	portH[4] = GPIOH->AFR[0];
	portH[5] = GPIOH->AFR[1];
#endif
}

void sleep_configure_io() {
#if defined(TARGET_XDOT_L151CC)
    // GPIO Ports Clock Enable
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOH_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    // UART1_TX, UART1_RTS & UART1_CTS to analog nopull - RX could be a wakeup source
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // I2C_SDA & I2C_SCL to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SPI_MOSI, SPI_MISO, SPI_SCK, & SPI_NSS to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // iterate through potential wake pins - leave the configured wake pin alone if one is needed
    if (dot->getWakePin() != WAKE || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO0 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO1 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO2 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != GPIO3 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    if (dot->getWakePin() != UART1_RX || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
#elif defined(TARGET_XDOT_MAX32670)
    LowPower::configExtGpios(dot->getWakeMode(), dot->getWakePin());
#else
    /* GPIO Ports Clock Enable */
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;

    // XBEE_DOUT, XBEE_DIN, XBEE_DO8, XBEE_RSSI, USBTX, USBRX, PA_12, PA_13, PA_14 & PA_15 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10
                | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PB_0, PB_1, PB_3 & PB_4 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PC_9 & PC_13 to analog nopull
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // iterate through potential wake pins - leave the configured wake pin alone if one is needed
    // XBEE_DIN - PA3
    // XBEE_DIO2 - PA5
    // XBEE_DIO3 - PA4
    // XBEE_DIO4 - PA7
    // XBEE_DIO5 - PC1
    // XBEE_DIO6 - PA1
    // XBEE_DIO7 - PA0
    // XBEE_SLEEPRQ - PA11

    if (dot->getWakePin() != XBEE_DIN || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (dot->getWakePin() != XBEE_DIO2 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if (dot->getWakePin() != XBEE_DIO3 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

         if (dot->getWakePin() != XBEE_DIO4 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO5 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO6 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_DIO7 || dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

     if (dot->getWakePin() != XBEE_SLEEPRQ|| dot->getWakeMode() == mDot::RTC_ALARM) {
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
#endif
}

void sleep_restore_io() {
#if defined(TARGET_XDOT_L151CC)
    xdot_restore_gpio_state();
#elif defined(TARGET_XDOT_MAX32670)
    // restored by sleep
#else
    GPIOA->MODER = portA[0];
    GPIOA->OTYPER = portA[1];
    GPIOA->OSPEEDR = portA[2];
    GPIOA->PUPDR = portA[3];
    GPIOA->AFR[0] = portA[4];
    GPIOA->AFR[1] = portA[5];

    GPIOB->MODER = portB[0];
    GPIOB->OTYPER = portB[1];
    GPIOB->OSPEEDR = portB[2];
    GPIOB->PUPDR = portB[3];
    GPIOB->AFR[0] = portB[4];
    GPIOB->AFR[1] = portB[5];

    GPIOC->MODER = portC[0];
    GPIOC->OTYPER = portC[1];
    GPIOC->OSPEEDR = portC[2];
    GPIOC->PUPDR = portC[3];
    GPIOC->AFR[0] = portC[4];
    GPIOC->AFR[1] = portC[5];

    GPIOD->MODER = portD[0];
    GPIOD->OTYPER = portD[1];
    GPIOD->OSPEEDR = portD[2];
    GPIOD->PUPDR = portD[3];
    GPIOD->AFR[0] = portD[4];
    GPIOD->AFR[1] = portD[5];

    GPIOH->MODER = portH[0];
    GPIOH->OTYPER = portH[1];
    GPIOH->OSPEEDR = portH[2];
    GPIOH->PUPDR = portH[3];
    GPIOH->AFR[0] = portH[4];
    GPIOH->AFR[1] = portH[5];
#endif
}

void read_sensor(std::vector<uint8_t> &tx_data) {
    uint16_t light;

#if defined(TARGET_XDOT_L151CC)
    // configure the ISL29011 sensor on the xDot-DK for continuous ambient light sampling, 16 bit conversion, and maximum range
    lux.setMode(ISL29011::ALS_CONT);
    lux.setResolution(ISL29011::ADC_16BIT);
    lux.setRange(ISL29011::RNG_64000);

    // get the latest light sample
    light = lux.getData();
    tx_data.push_back((light >> 8) & 0xFF);
    tx_data.push_back(light & 0xFF);
    logInfo("light: %lu [0x%04X]", light, light);
    // put the LSL29011 ambient light sensor into a low power state
    lux.setMode(ISL29011::PWR_DOWN);
#elif defined(TARGET_XDOT_MAX32670)
    // get some dummy data
    light = rand();
    tx_data.push_back((light >> 8) & 0xFF);
    tx_data.push_back(light & 0xFF);
    logInfo("light: %lu [0x%04X]", light, light);
#else
    // get some dummy data
    light = lux.read_u16();
    tx_data.push_back((light >> 8) & 0xFF);
    tx_data.push_back(light & 0xFF);
    logInfo("light: %lu [0x%04X]", light, light);
#endif

}

// Puts entire device to sleep. No active threads.
void dot_wait_for_channel() {
    uint32_t next_tx_ms = dot->getNextTxMs();
    if (next_tx_ms) {
        logInfo("Sleep until next available channel");
        sleep_wake_rtc_only(next_tx_ms/1000+1, false);
    }
}

// Allows other threads to run including downlink processing in RadioEvents.
void thread_wait_for_channel() {
    ThisThread::sleep_for(chrono::milliseconds(dot->getNextTxMs()));
}

int send(uint8_t &size_sent) {
    int32_t ret;
    std::vector<uint8_t> tx_data;

    read_sensor(tx_data);
    // Make sure there is enough room for the payload. For US915 DR0, it is limited to 11 and MAC commands may consume
    // some of that space. Sending with no payload will send and clear the MAC commands freeing the payload space.
    if (dot->getNextTxMaxSize() < tx_data.size()) {
        logWarning("Not enough room for payload. Sending empty payload to clear MAC commands.");
        tx_data.clear();
    }
    size_sent = tx_data.size();

    ret = dot->send(tx_data);
    if (ret != mDot::MDOT_OK) {
        logWarning("failed to send data to %s [%d][%s]", dot->getJoinMode() == mDot::PEER_TO_PEER ? "peer" : "gateway", ret, mDot::getReturnCodeString(ret).c_str());
    } else {
        logInfo("successfully sent data to %s", dot->getJoinMode() == mDot::PEER_TO_PEER ? "peer" : "gateway");
    }

    return ret;
}
