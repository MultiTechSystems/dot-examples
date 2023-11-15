#include "dot_util.h"
#include "RadioEvent.h"
#include "library_version.h"

#if ACTIVE_EXAMPLE == LCTT_EXAMPLE

mDot* dot = NULL;
lora::ChannelPlan* plan = NULL;

mbed::UnbufferedSerial pc(USBTX, USBRX);


void RadioEvent::handleTestModePacket()
 {
    // static uint32_t last_rx_seq = 0;  // For Class B tests
    // bool start_test = false;

    std::string packet = mts::Text::bin2hexString(RxPayload, RxPayloadSize);

    testModeEnabled = true;
    dot->setTestModeEnabled(true);

    // last_rx_seq = dot->getSettings()->Session.UplinkCounter; // for class b tests

    uint32_t txPeriod = 5000;
    bool testConfirmed = false;

    std::string cls = "A";

    Timer sentTimer;
    sentTimer.start();
    while (testModeEnabled) {
logDebug("TEST_START");
TEST_START:
        // if (waitingForBeacon && BeaconLocked) {
        //     logInfo("send BeaconRxStatusInd");
        //     _data.clear();
        //     _data.push_back(0x40);
        //     for (size_t i = 0; i < sizeof(BeaconData); ++i) {
        //         _data.push_back(((uint8_t*)&BeaconData)[i]);
        //     }
        // } else

        if (RxPort == 224) {
            _data.clear();

            std::string packet = mts::Text::bin2hexString(RxPayload, RxPayloadSize);
            logDebug("Test Mode AppPort : %d", dot->getAppPort());
            logDebug("Test Mode Packet : %s", packet.c_str());

            switch (RxPayload[0]) {
                case 0x00: { // PackageVersionReq
                    _data.push_back(0x00);
                    _data.push_back(0x06);
                    _data.push_back(0x01);
                    break;
                }
                case 0x01: { // DutResetReq
                    if (RxPayloadSize == 1) {
                        dot->resetCpu();
                    }
                    break;
                }
                case 0x02: { // DutJoinReq
                    if (RxPayloadSize == 1) {
                        while (dot->getNextTxMs() > 0) {
                            osDelay(1000);
                        }
                        dot->joinNetworkOnce();
                        testModeEnabled = false;
                        return;
                    }
                    break;
                }
                case 0x03: { // SwitchClassReq
                    if (RxPayload[1] < 3) {
                        if (RxPayload[1] == 0x00) {
                            cls = "A";
                        } else {
                            cls = RxPayload[1] == 0x01 ? "B" : "C";
                        }
                    }
                    dot->setClass(cls);
                    break;
                }
                case 0x04: { // ADR Enabled/Disable
                    if (RxPayload[1] == 1)
                        dot->setAdr(true);
                    else
                        dot->setAdr(false);
                    break;
                }
                case 0x05: { // RegionalDutyCycleCtrlReq
                    if (RxPayload[1] == 0)
                        dot->setDisableDutyCycle(true);
                    else
                        dot->setDisableDutyCycle(false);
                    break;
                }
                case 0x06: { // TxPeriodicityChangeReq
                    if (RxPayload[1] < 2)
                        // 0, 1 => 5s
                        txPeriod = 5000U;
                    else if (RxPayload[1] < 8)
                        // 2 - 7 => 10s - 60s
                        txPeriod = (RxPayload[1] - 1) * 10000U;
                    else if (RxPayload[1] < 11) {
                        // 8, 9, 10 => 120s, 240s, 480s
                        txPeriod = 120 * (1 << (RxPayload[1] - 8)) * 1000U;
                    }
                    break;
                }
                case 0x07: { // TxFramesCtrl
                    if (RxPayload[1] == 0) {
                        // NO-OP
                    } else if (RxPayload[1] == 1) {
                        testConfirmed = false;
                        dot->getSettings()->Network.AckEnabled = 0;
                    } else if (RxPayload[1] == 2) {
                        testConfirmed = true;
                        // if ADR has set nbTrans then use the current setting
                        dot->getSettings()->Network.AckEnabled = 1;
                        if (dot->getSettings()->Session.Redundancy == 0) {
                            dot->getSettings()->Session.Redundancy = 1;
                        }
                    }
                    break;
                }
                case 0x08: { // EchoPayloadReq
                    _data.push_back(0x08);
                    for (size_t i = 1; i < RxPayloadSize; i++) {
                        _data.push_back(RxPayload[i] + 1);
                    }
                    break;
                }
                case 0x09: { // RxAppCntReq
                    _data.push_back(0x09);
                    _data.push_back(_testDownlinkCounter & 0xFF);
                    _data.push_back(_testDownlinkCounter >> 8);
                    break;
                }
                case 0x0A: { // RxAppCntResetReq
                    _testDownlinkCounter = 0;
                    break;
                }
                case 0x20: { // LinkCheckReq
                    dot->addMacCommand(lora::MOTE_MAC_LINK_CHECK_REQ, 0, 0);
                    break;
                }
                case 0x21: { // DeviceTimeReq
                    dot->addDeviceTimeRequest();
                    break;
                }
                case 0x22: { // PingSlotInfo
                    dot->setPingPeriodicity(RxPayload[1]);
                    dot->addMacCommand(lora::MOTE_MAC_PING_SLOT_INFO_REQ, RxPayload[1], 0);
                    break;
                }
                case 0x7D: { // TxCw
                    uint32_t freq = 0;
                    uint16_t timeout = 0;
                    uint8_t power = 0;

                    timeout = RxPayload[2] << 8 | RxPayload[1];
                    freq = (RxPayload[5] << 16 | RxPayload[4] << 8 | RxPayload[2]) * 100;
                    power = RxPayload[6];

                    dot->sendContinuous(true, timeout * 1000, freq, power);
                    break;
                }
                case 0x7E: { // DutFPort224DisableReq
                    testModeEnabled = false;
                    dot->resetCpu();
                    break;
                }
                case 0x7F: { // DutVersionReq
                    std::string version = MDOT_VERSION;
                    int temp = 0;

                    _data.push_back(0x7F);
                    sscanf(&version[0], "%d", &temp);
                    _data.push_back(temp); // AT_APP_VERSION_MAJOR; // MAJOR
                    sscanf(&version[2], "%d", &temp);
                    _data.push_back(temp); // AT_APP_VERSION_MINOR; // MINOR
                    sscanf(&version[4], "%d", &temp);
                    _data.push_back(temp); // AT_APP_VERSION_PATCH; // PATCH
                    if (version.size() > 7) {
                        sscanf(&version[6], "%d", &temp);
                        _data.push_back(temp); // AT_APP_VERSION_PATCH; // PATCH
                    } else {
                        _data.push_back(0); // AT_APP_VERSION_PATCH; // PATCH
                    }
                    version = LW_VERSION;
                    sscanf(&version[0], "%d", &temp);
                    _data.push_back(temp); // LW_VERSION; // MAJOR
                    sscanf(&version[2], "%d", &temp);
                    _data.push_back(temp); // LW_VERSION; // MINOR
                    sscanf(&version[4], "%d", &temp);
                    _data.push_back(temp); // LW_VERSION; // PATCH
                    if (version.size() > 7) {
                        sscanf(&version[6], "%d", &temp);
                        _data.push_back(temp); // LW_VERSION; // PATCH
                    } else {
                        _data.push_back(0); // LW_VERSION; // PATCH
                    }
                    version = RP_VERSION;
                    sscanf(&version[0], "%d", &temp);
                    _data.push_back(temp); // RP_VERSION; // MAJOR
                    sscanf(&version[2], "%d", &temp);
                    _data.push_back(temp); // RP_VERSION; // MINOR
                    sscanf(&version[4], "%d", &temp);
                    _data.push_back(temp); // RP_VERSION; // PATCH
                    if (version.size() > 7) {
                        sscanf(&version[6], "%d", &temp);
                        _data.push_back(temp); // RP_VERSION; // PATCH
                    } else {
                        _data.push_back(0); // RP_VERSION; // PATCH
                    }


                    break;
                }
                default: {
                    break;
                }
            }
        }

        do {
            uint32_t loop_timer = 0;
            loop_timer = std::chrono::duration_cast<std::chrono::milliseconds>(sentTimer.elapsed_time()).count();

            PacketReceived = false;
            AckReceived = false;

            while (loop_timer < txPeriod || dot->getNextTxMs() > 0 || !dot->getIsIdle()) {
                // if (waitingForBeacon && BeaconLocked) {
                //     goto TEST_START;
                // }
                osDelay(500);
                loop_timer = std::chrono::duration_cast<std::chrono::milliseconds>(sentTimer.elapsed_time()).count();

                if (dot->getAckRequested() && loop_timer > 5000) {
                    break;
                }
            }

            // packet may have been received while waiting, if ACK req then send uplink
            // else process data
            if (!PacketReceived || dot->getAckRequested()) {

                sentTimer.reset();

                if (_data.size() == 0) {
                    dot->setAppPort(1);
                    // if (cls == "B") {
                    //     // class B tests don't like empty packets
                        _data.push_back(0xFF);
                    // }
                } else {
                    dot->setAppPort(224);
                }

                PacketReceived = false;
                AckReceived = false;

                logDebug("testmode send");

                if (dot->send(_data, testConfirmed) == mDot::MDOT_MAX_PAYLOAD_EXCEEDED) {
                    _data.clear();
                    RxPort = 0;
                    goto TEST_START;
                }

                // For Class B tests
                // if (PacketReceived) {
                //     last_rx_seq = dot->getSettings()->Session.UplinkCounter;
                // }

                _data.clear();

                logDebug("testmode end loop");
            }

        } while (!AckReceived && dot->recv(_data) != mDot::MDOT_OK);


        if (cls == "B" && !BeaconLocked) {
            dot->setClass(cls);
        }
    }
}

#if defined(TARGET_MTS_MDOT_F411RE) // -----------------------------------------------------------

#define AT_TX_PIN   XBEE_DOUT
#define AT_RX_PIN   XBEE_DIN
#define AT_RTS_PIN  XBEE_RTS
#define AT_CTS_PIN  XBEE_CTS

#define DEBUG_TX    USBTX
#define DEBUG_RX    USBRX

#elif defined(TARGET_XDOT_L151CC) // -----------------------------------------------------------

#define AT_TX_PIN   UART1_TX
#define AT_RX_PIN   UART1_RX
#define AT_RTS_PIN  UART1_RTS
#define AT_CTS_PIN  UART1_CTS

#define DEBUG_TX    USBTX
#define DEBUG_RX    USBRX

#elif defined(TARGET_XDOT_MAX32670) // -----------------------------------------------------------

#define AT_TX_PIN   UART0_TX // UART0A_TX - P0_9
#define AT_RX_PIN   UART0_RX // UART0A_RX - P0_8
#define AT_RTS_PIN  UART0_RTS
#define AT_CTS_PIN  UART0_CTS

#define DEBUG_TX    UART1_TX // UART1A_TX - P0_29
#define DEBUG_RX    UART1_RX // UART1A_RX - P0_28

#include "pwrseq_regs.h"
#include "lp.h"

#else // -----------------------------------------------------------------------------------------
#error Unsupported target
#endif

#define LOG_DEFAULT_BAUD_RATE 115200

mbed::UnbufferedSerial debug_port(DEBUG_TX, DEBUG_RX, LOG_DEFAULT_BAUD_RATE);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &debug_port;
}


int main() {
    // Custom event handler for automatically displaying RX data
    RadioEvent events;

    pc.baud(115200);

#if defined(TARGET_XDOT_L151CC)
    i2c.frequency(400000);
#endif

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    logInfo("----- Running LCTT example -----");

    // Create channel plan
    plan = create_channel_plan();
    assert(plan);

    dot = mDot::getInstance(plan);
    assert(dot);

    // attach the custom events handler
    dot->setEvents(&events);

    // Enable FOTA for multicast support
    Fota::getInstance(dot);

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

    if (!dot->getStandbyFlag() && !dot->getPreserveSession()) {

        dot->setJoinMode(mDot::OTA);
#if defined(DERIVE_FROM_TEXT)
        update_ota_config_name_phrase(cfg::network_name, cfg::network_passphrase, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#else
        update_ota_config_id_key(cfg::network_id, cfg::network_key, cfg::frequency_sub_band, cfg::network_type, cfg::ack);
#endif

        logInfo("saving configuration");
        if (!dot->saveConfig()) {
            logError("failed to save configuration");
        }
    }

    dot->setClass("A");

    dot->setTestModeEnabled(true);

    // For library 4.2.x use setDisableIncrementDR
    dot->setDisableIncrementDR(true);

    // For library 4.1.x use Test.DisableADRIncrementDatarate
    // dot->getSettings()->Test.DisableADRIncrementDatarate = true;


    dot->setJoinNonceValidation(true);
    dot->setLinkCheckThreshold(0);
    dot->setLinkCheckCount(0);
    dot->setAppPort(224);
    // These intentionally override the settings in example_config.h
    dot->setFrequencySubBand(1); // US915/AU915 8 channel test, set to 0 for 64 channel tests
    dot->setAck(0);
    dot->setAdr(true);

    // display configuration
    display_config();

    mts::MTSLog::setLogLevel(mts::MTSLog::TRACE_LEVEL);

//Run terminal session
    while (true) {
        if (!events.testModeEnabled) {
            logDebug("NOT IN TESTMODE, run main thread");
            if (dot->getNextTxMs() > 0) {
                logDebug("Backoff %d s", dot->getNextTxMs()/1000);
                osDelay(2000);
            } else if (!dot->getNetworkJoinStatus()) {
                if (dot->joinNetworkOnce() != mDot::MDOT_OK) {
                    logDebug("Network Not Joined\r\n");
                }
            } else if (events.PacketReceived) {
                logDebug("ENTER TEST MODE *************");
                events.handleTestModePacket();
                logDebug("********** TEST MODE RETURNED");
                events.PacketReceived =  false;
            } else  {
                logDebug("main.cpp send");
                std::vector<uint8_t> data;
                dot->send(data, false);
            }
        } else {
            logDebug("IN TESTMODE, sleep main thread");
        }

        ThisThread::sleep_for(5s);
    }


    return 0;
}

#endif
