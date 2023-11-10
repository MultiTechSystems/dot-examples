#ifndef __EXAMPLE__CONFIG_H__
#define __EXAMPLE__CONFIG_H__

/////////////////////////////////////////////////////////////////////////////
//--------------------- DOT LIBRARY REQUIRED ------------------------------//
// * Because these example programs can be used for mDot, xDot_L151CC and  //
//     xDot_MAX32670, the LoRa stack is not included. You must add one of  //
//     the following libraries depending on the module and if you want to  //
//     use the latest development branch or the stable branch.             //
//  -------------- In the directory dot-examples/examples ---------------  //
// * For mDot modules:                                                     //
//     mbed add https://github.com/MultiTechSystems/libmDot-dev            //
//     mbed add https://github.com/MultiTechSystems/libmDot                //
// * For xDot_L151CC modules:                                              //
//     mbed add https://github.com/MultiTechSystems/libxDot-dev            //
//     mbed add https://github.com/MultiTechSystems/libxDot                //
// * For xDot_MAX32670 modules                                             //
//     mbed add https://github.com/MultiTechSystems/libxDotAD-dev          //
//     mbed add https://github.com/MultiTechSystems/libxDotAD              //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//----------------- REQUIRED MBED-OS AND COMPILER VERSIONS ----------------//
// * The above dot libraries where compiled with a specific version of     //
//     mbed-os and specific version of the GCC_ARM and ARMC6 compiler.     //
//     You must build the dot-examples or your custom application with     //
//     compatible versions of mbed-os and the compiler.                    //
// * Determining the proper mbed-os version.                               //
//     Check the commit message of the specific dot library that you       //
//     added. For example:                                                 //
//     1. Go to https://github.com/MultiTechSystems/libxDotAD-dev          //
//     2. Click on commits                                                 //
//     3. Click on the latest commit to see the full commit text:          //
//       xdotad-library revision 4.2.4 and mbed-os revision mbed-os-       //
//       6.17.0-rc3……-90-g72f27cee92                                       //
//     4. Checkout commit 72f27cee92                                       //
// * Determining the proper compiler version.                              //
//     See the README.md text for the dot library added. For exmaple:      //
//     1. Go to https://github.com/MultiTechSystems/libxDotAD-dev          //
//     2. The README.md file contents are displayed below the list of      //
//        files and lists the GCC_ARM and ARMC6 compiler versions.         //
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// * Choose which example to build. This can be done on the            //
//   command line when compiling the example or selected below.        //
//   -- command line example --                                        //
// mbed compile -t ARMC6 -m XDOT_MAX32670 -DACTIVE_EXAMPLE=OTA_EXAMPLE //
/////////////////////////////////////////////////////////////////////////
#define OTA_EXAMPLE              1  // see ota_example.cpp
#define AUTO_OTA_EXAMPLE         2  // see auto_ota_example.cpp
#define MANUAL_EXAMPLE           3  // see manual_example.cpp
#define PEER_TO_PEER_EXAMPLE     4  // see peer_to_peer_example.cpp
#define CLASS_C_EXAMPLE          5  // see class_c_example.cpp
#define CLASS_B_EXAMPLE          6  // see class_b_example.cpp
#define FOTA_EXAMPLE             7  // see fota_example.cpp
#define LCTT_EXAMPLE             8  // see lctt_example.cpp

#if !defined(ACTIVE_EXAMPLE)
#define ACTIVE_EXAMPLE  LCTT_EXAMPLE
#endif

namespace cfg {

/////////////////////////////////////////////////////////////
// * Configure the following settings to match the gateway //
//     the endpoint will be communicating with.            //
// * Frequency sub band is only relevant for 915 bands.    //
// * Configure either the network name and passphrase or   //
//     the network ID (8 bytes) and KEY (16 bytes)         //
/////////////////////////////////////////////////////////////
#if (ACTIVE_EXAMPLE == MANUAL_EXAMPLE || ACTIVE_EXAMPLE == PEER_TO_PEER_EXAMPLE)
const uint8_t network_address[] = { 0x01, 0x02, 0x03, 0x04 };
const uint8_t network_session_key[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
const uint8_t data_session_key[] = { 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04 };
#else // All other examples use the AppEUI and AppKey
// Choose how to set the AppEUI and AppKey.
#define DERIVE_FROM_TEXT
#if defined(DERIVE_FROM_TEXT)
// AppEUI
const std::string network_name = "MTS-SKF-2022";
// AppKey
const std::string network_passphrase = "MTS-SKF-2022";
#else // use kex keys instead of deriving from text
// AppEUI
const uint8_t network_id[] = { 0x6C, 0x4E, 0xEF, 0x66, 0xF4, 0x79, 0x86, 0xA6 };
// AppKey
const uint8_t network_key[] = { 0x1F, 0x33, 0xA1, 0x70, 0xA5, 0xF1, 0xFD, 0xA0, 0xAB, 0x69, 0x7A, 0xAE, 0x2B, 0x95, 0x91, 0x6B };
#endif // defined(DERIVE_FROM_TEXT)
#endif // (ACTIVE_EXAMPLE == MANUAL_EXAMPLE || ACTIVE_EXAMPLE == PEER_TO_PEER_EXAMPLE)
const uint8_t frequency_sub_band = 1;
const lora::NetworkType network_type = lora::PUBLIC_LORAWAN;
const uint8_t join_delay = 5;
const bool adr = true;
const uint8_t ack = 0;

//////////////////////////////////////////////////////////////
// * Configure sleep mode settings.                         //
// * For xDot modules, deep sleep consumes less current     //
//     than sleep.                                          //
// * The xDot_MAX32670 also consumes less power if waking   //
//     from interrupt vs RTC/interval.                      //
// * For all modules in sleep mode, IO state is maintained, //
//     RAM is retained, and the application will resume     //
//     after waking up.                                     //
// * For xDot_L151CC and mDot in deepsleep mode, IOs float, //
//     RAM is lost, and the application will start from the //
//     beginning after waking up.                           //
// * For the xDot_MAX32670 in deepsleep mode, IO state is   //
//     maintainded, RAM is lost, and the application        //
//     restarts after waking.                               //
// * If deep_sleep == true, device will enter deepsleep     //
//     mode.                                                //
//////////////////////////////////////////////////////////////
const bool deep_sleep = false;

////////////////////////////////////////////////////////////////
// * Configure wake mode.                                     //
//     0 = Interval, 1 = Interrupt, 2 = Interval or interrupt //
//     Wake on interval is based on the variable delay_s in   //
//     the file dot_util.cpp.                                 //
// * Configure wake pin.                                      //
//     Processor pins can have multiple aliases defined in    //
//     mbed-os. See the PinNames.h file for all.              //
//     ------- mDot -------     ---- xDot ----                //
//     1 = PA_3, XBEE_DIN         1 = UART_RX                 //
//     2 = PA_5, XBEE_DIO2        2 = GPIO0                   //
//     3 = PA_4, XBEE_DIO3        3 = GPIO1                   //
//     4 = PA_7, XBEE_DIO4        4 = GPIO2                   //
//     5 = PC_1, XBEE_DIO5        5 = GPIO3                   //
//     6 = PA_1, XBEE_DIO6        6 = WAKE                    //
//     7 = PA_0, XBEE_DIO7                                    //
//     8 = PA_11, XBEE_DI8                                    //
// * Configure wake pin mode.                                 //
//     0 = No pull, 1 = pull up, 2 = pull down                //
// * Configure wake pin trigger.                              //
//     0 = Rise or fall, 1 = Rise, 2 = Fall                   //
////////////////////////////////////////////////////////////////
enum WakeMode {interval, interrupt, interval_or_interrupt};
const uint8_t wake_mode = interrupt;

#if defined(TARGET_MTS_MDOT_F411RE)
enum WakePin {xbee_din=1, xbee_dio2, xbee_dio3, xbee_dio4, xbee_dio5, xbee_dio6, xbee_dio7, xbee_di8};
// For deepsleep, the mdot wake pin must be xbee_dio7.
const uint8_t wake_pin = xbee_dio7;
#elif defined(TARGET_XDOT_L151CC) || defined(TARGET_XDOT_MAX32670)
enum WakePin {uart_rx=1, gpio0, gpio1, gpio2, gpio3, wake};
// For deepsleep, the xDot_L151CC wake pin must be 6/WAKE.
// Push button switch S2 on the xDot DK board connects to the wake pin.
const uint8_t wake_pin = wake;
#endif

// The xDot DK board connects the wake pin to a push button switch S2. When
//    pushed, the wake pin is connected to the power rail.
// PinMode is defined in mbed-os for target devices. Options include
//    PullNone, PullUp, PullDown and others depending on target.
const PinMode wake_pin_mode = PullDown;
enum WakePinTrigger{RISE_OR_FALL, RISE, FALL};
const uint8_t wake_pin_trigger = RISE;

} // end of cfg namespace

//////////////////////////////////////////////////////////////////
// * Configure the channel pland/region.                        //
// * The Channel Plan should be chosen with command line        //
//     arguments or in the mbed_app.json file macros            //
//     section as follows:                                      //
//                                                              //
//  -- command line example --                                  //
// mbed compile -t ARMC6 -m XDOT_L151CC -DCHANNEL_PLAN=CP_US915 //
//                                                              //
//  -- mbed_app.json example --                                 //
//  "macros": [                                                 //
//     "FOTA",                                                  //
//     "CHANNEL_PLAN=CP_AS923_2"                                //
// ],                                                           //
// Or for global plan                                           //
//  "macros": [                                                 //
//     "FOTA",                                                  //
//     "CHANNEL_PLAN=CP_GLOBAL"                                 //
//     "GLOBAL_PLAN=CP_US915"                                   //
 // ],                                                          //
//                                                              //
//  -- Channel plan options are: --                             //
//      CP_US915                                                //
//      CP_AU915                                                //
//      CP_EU868                                                //
//      CP_KR920                                                //
//      CP_AS923                                                //
//      CP_AS923_2                                              //
//      CP_AS923_3                                              //
//      CP_AS923_4                                              //
//      CP_AS923_JAPAN                                          //
//      CP_AS923_JAPAN1                                         //
//      CP_AS923_JAPAN2                                         //
//      CP_IN865                                                //
//      CP_RU864                                                //
//////////////////////////////////////////////////////////////////

// This default is only used if the channel plan is not defined in
//   mbed_app.json or on the command line during compilation.
#if !defined(CHANNEL_PLAN)
#define CHANNEL_PLAN CP_US915
#endif

#endif
