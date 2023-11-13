#ifndef __RADIO_EVENT_H__
#define __RADIO_EVENT_H__

#include "dot_util.h"
#include "mDotEvent.h"
#include "Fota.h"
#include "example_config.h"

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

            printf("Downlink payload (port %d): %s\r\n", port, mts::Text::bin2hexString(_data.data(), _data.size()).c_str());
        }

        if(port == 200 || port == 201 || port == 202) {
            Fota::getInstance()->processCmd(payload, port, size);
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
};

#endif

