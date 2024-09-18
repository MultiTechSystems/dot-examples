#ifndef __RADIO_EVENT_H__
#define __RADIO_EVENT_H__

#include "dot_util.h"
#include "mDotEvent.h"
#include "Fota.h"
#include "example_config.h"

extern mDot* dot;

class RadioEvent : public mDotEvent
{

public:
    bool joined = false;
    bool testModeEnabled = false;
    std::vector<uint8_t> _data;
    uint32_t _testDownlinkCounter;

    RadioEvent() {}

    virtual ~RadioEvent() {}

    virtual void PacketRx(uint8_t port, uint8_t *payload, uint16_t size, int16_t rssi, int16_t snr, lora::DownlinkControl ctrl, uint8_t slot, uint8_t retries, uint32_t address, uint32_t fcnt, bool dupRx) {
        mDotEvent::PacketRx(port, payload, size, rssi, snr, ctrl, slot, retries, address, fcnt, dupRx);

        // Downlink of payload can be processed here. Port 1 is the default. Other ports are valid to use.
        // Check latest LoRaWAN spec for available ports.
        if (port==(dot->getAppPort())) {
            _data.clear();
            
            for (uint16_t i = 0; i < size; ++i) {
                _data.push_back(payload[i]);
            }
        }

        if (port == 200 || port == 201 || port == 202) {
            Fota::getInstance()->processCmd(payload, port, size);
            if (port == 202) {
                // Parse payload looking for ForceDeviceResyncReq command.
                uint8_t clock_sync_command;
                uint8_t i = 0;
                while (i < size) {
                    clock_sync_command = payload[i];
                    switch (clock_sync_command) {
                        case 0: {   // PackageVersionReq
                            i++;    // There is no payload for the PackageVersionReqdecrement_clock_correction_retries
                            break;
                        }
                        case 1: {   // AppTimeAns
                            //logInfo("Received AppTimeAns");
                            force_device_resync_req = false;
                            int32_t time_correction =
                                    payload[i+4] << 24 |
                                    payload[i+3] << 16 |
                                    payload[i+2] << 8 |
                                    payload[i+1];
                            // Adjust the time if the token matches.
                            if ((payload[i+5] & 0x0f) == token_req) {
                                // get the time
                                // 315964800U - GPS offset - Unix time epoch (01 Jan 1970) vs GPS time epoch (06 Jan 1980)
                                uint32_t gpsTime = Fota::getInstance()->getClockOffset() + time(NULL) - 315964800U;
                                if (time_correction > 0){
                                    // add the correction
                                    gpsTime += time_correction;
                                } else {
                                    // subtract the correction
                                    gpsTime += -time_correction;
                                }
                                // save the time
                                Fota::getInstance()->setClockOffset(gpsTime);
                            }
                            token_req++;
                            i+=6;    // Payload is 5 bytes
                            break;
                        }
                        case 2: {    // DeviceAppTimePeriodicityReq
                            i+=2;   // Payload is 1 byte
                            break;
                        }
                        case 3: {    // ForceDeviceResyncReq
                            // The clock sync spec says to discard the command silently if NbTrans=0
                            force_device_resync_req_nbTrans = payload[i+1] & 0x07;
                            //logInfo("Received ForceDeviceResyncReq, NbTrans = %d", force_device_resync_req_nbTrans);
                            if (force_device_resync_req_nbTrans == 0) {
                                force_device_resync_req = false;
                            } else {
                                force_device_resync_req = true;
                            }
                            i+=2;   // Payload is 1 byte
                            break;
                        }
                    }
                }
            }
        }

        if (testModeEnabled) {
            if (AckReceived || (PacketReceived && (RxPort != 0 || RxPayloadSize == 0))) {
                _testDownlinkCounter++;
                logDebug("Incremented downlink cnt %d", _testDownlinkCounter);
            }
        }
    }

    /*!
     * MAC layer event callback prototype.
     *
     * \param [IN] flags Bit field indicating the MAC events occurred
     * \param [IN] info  Details about MAC events occurred
     */
    virtual void MacEvent(LoRaMacEventFlags* flags, LoRaMacEventInfo* info) {

        if (mts::MTSLog::getLogLevel() == mts::MTSLog::TRACE_LEVEL) {
            std::string msg = "OK";
            switch (info->Status) {
                case LORAMAC_EVENT_INFO_STATUS_ERROR:
                    msg = "ERROR";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT:
                    msg = "TX_TIMEOUT";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_RX_TIMEOUT:
                    msg = "RX_TIMEOUT";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_RX_ERROR:
                    msg = "RX_ERROR";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL:
                    msg = "JOIN_FAIL";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_DOWNLINK_FAIL:
                    msg = "DOWNLINK_FAIL";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL:
                    msg = "ADDRESS_FAIL";
                    break;
                case LORAMAC_EVENT_INFO_STATUS_MIC_FAIL:
                    msg = "MIC_FAIL";
                    break;
                default:
                    break;
            }
            logTrace("Event: %s", msg.c_str());

            logTrace("Flags Tx: %d Rx: %d RxData: %d RxSlot: %d LinkCheck: %d JoinAccept: %d",
                     flags->Bits.Tx, flags->Bits.Rx, flags->Bits.RxData, flags->Bits.RxSlot, flags->Bits.LinkCheck, flags->Bits.JoinAccept);
            logTrace("Info: Status: %d ACK: %d Retries: %d TxDR: %d RxPort: %d RxSize: %d RSSI: %d SNR: %d Energy: %d Margin: %d Gateways: %d",
                     info->Status, info->TxAckReceived, info->TxNbRetries, info->TxDatarate, info->RxPort, info->RxBufferSize,
                     info->RxRssi, info->RxSnr, info->Energy, info->DemodMargin, info->NbGateways);
        }

        if (flags->Bits.Rx) {

            logInfo("Rx %d bytes", info->RxBufferSize);

            if (info->RxBufferSize > 0) {
                // Check for rejoin command from gateway
                if (info->RxPort == 1 && info->RxBufferSize == 1 && info->RxBuffer[0] == 0xFF) {
                    joined = false;
                }

#if ACTIVE_EXAMPLE != FOTA_EXAMPLE
                // print RX data as string and hexadecimal
                // std::string rx((const char*)info->RxBuffer, info->RxBufferSize);
                // printf("Rx data: [%s]\r\n", mts::Text::bin2hexString(info->RxBuffer, info->RxBufferSize).c_str());
#endif
            }
        }
    }

#if ACTIVE_EXAMPLE == LCTT_EXAMPLE
    void handleTestModePacket();
#endif

    virtual void ServerTime(uint32_t seconds, uint8_t sub_seconds) {
        mDotEvent::ServerTime(seconds, sub_seconds);

        Fota::getInstance()->setClockOffset(seconds);
    }

    void decrement_clock_correction_retries() {
        if (force_device_resync_req_nbTrans > 0)
            force_device_resync_req_nbTrans--;
    }

    uint8_t get_clock_correction_retries() {
        return force_device_resync_req_nbTrans;
    }

    void clear_clock_resync_req() {
        force_device_resync_req = false;
    }

    bool get_clock_resync_req() {
        return force_device_resync_req;
    }

    uint8_t get_token_req() {
        return token_req;
    }

private:
    bool force_device_resync_req = false;
    uint8_t force_device_resync_req_nbTrans = 0;
    uint8_t token_req = 0;
};

#endif

