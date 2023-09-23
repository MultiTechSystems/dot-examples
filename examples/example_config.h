#ifndef __EXAMPLE__CONFIG_H__
#define __EXAMPLE__CONFIG_H__

#define OTA_EXAMPLE              1  // see ota_example.cpp
#define AUTO_OTA_EXAMPLE         2  // see auto_ota_example.cpp
#define MANUAL_EXAMPLE           3  // see manual_example.cpp
#define PEER_TO_PEER_EXAMPLE     4  // see peer_to_peer_example.cpp
#define CLASS_C_EXAMPLE          5  // see class_c_example.cpp
#define CLASS_B_EXAMPLE          6  // see class_b_example.cpp
#define FOTA_EXAMPLE             7  // see fota_example.cpp
#define LCTT_EXAMPLE             8  // see lctt_example.cpp

// the active example is the one that will be compiled
#if !defined(ACTIVE_EXAMPLE)
#define ACTIVE_EXAMPLE  LCTT_EXAMPLE
#endif

// the active channel plan is the one that will be compiled
// The Channel Plan should be chosen with command line arguments or in the mbed_app.json file macros section.

// -DCHANNEL_PLAN=CP_US915
// -DCHANNEL_PLAN=CP_AS923
// -DCHANNEL_PLAN=CP_AS923_3
// -DCHANNEL_PLAN=CP_AS923_JAPAN2

//  "macros": [
//     "FOTA",
//     "CHANNEL_PLAN=CP_AS923_2"
 // ],

// options are :
//      CP_US915
//      CP_AU915
//      CP_EU868
//      CP_KR920
//      CP_AS923
//      CP_AS923_2
//      CP_AS923_3
//      CP_AS923_4
//      CP_AS923_JAPAN
//      CP_AS923_JAPAN1
//      CP_AS923_JAPAN2
//      CP_IN865
//      CP_RU864

#if !defined(CHANNEL_PLAN)
#define CHANNEL_PLAN CP_US915
#endif

#endif
