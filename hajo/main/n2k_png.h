/*
 * Documents:
 *
 * https://copperhilltech.com/content/NMEA2K_Network_Design_v2.pdf
 * https://canboat.github.io/canboat/canboat.xml
 *
 */

/*
 * single/fast packet ranges
 *
 * PDU1 (addressed) single-frame PGN range 0E800 to 0xEEFF (59392 - 61183)
 * PDU1 (addressed) single-frame PGN range 0EF00 to 0xEFFF (61184 - 61439)
 * PDU2 non-addressed single-frame PGN range 0xF000 to 0xFEFF (61440 - 65279)
 * PDU2 (non-addressed) single-frame PGN range 0xFF00 to 0xFFFF (65280 - 65535)
 * PDU1 (addressed) fast-packet PGN range 0x10000 to 0x1EE00 (65536 - 126464)
 * PDU1 (addressed) fast-packet PGN range 0x1EF00 to 0x1EFFF (126720 - 126975)
 * PDU2 (non-addressed) mixed single/fast packet PGN range 0x1F000 to 0x1FEFF (126976 - 130815)
 * PDU2 (non-addressed) fast packet PGN range 0x1FF00 to 0x1FFFF (130816 - 131071)
 */

/*
 * Generic fields:
 *
 * SID: The sequence identifier field is used to tie related PGNs together.
 * For example, the SSC200 will transmit identical SIDs for Vessel Heading (PGN 127250),
 * Attitude (127257), and Rate of Turn (127251) to indicate that the readings are
 * linked together (i.e., the data from each PGN was taken at the same time although
 * they are reported at slightly different times).
 *
 * Instance: This field indicates the particular instance for which this data applies.
 *
 * Source, destination: different Unique number (set by SetDeviceInformation) and different Model serial code (set by SetProductInformation)
 */

/*
 * Lookup values
 */
#define N2K_DEVICE_CLASS_RESERVED_FOR_2000_USE 0
#define N2K_DEVICE_CLASS_SYSTEM_TOOLS 10
#define N2K_DEVICE_CLASS_SAFETY_SYSTEMS 20
#define N2K_DEVICE_CLASS_INTERNETWORK_DEVICE 25
#define N2K_DEVICE_CLASS_ELECTRICAL_DISTRIBUTION 30
#define N2K_DEVICE_CLASS_ELECTRICAL_GENERATION 35
#define N2K_DEVICE_CLASS_STEERING_AND_CONTROL_SURFACES 40
#define N2K_DEVICE_CLASS_PROPULSION 50
#define N2K_DEVICE_CLASS_NAVIGATION 60
#define N2K_DEVICE_CLASS_COMMUNICATION 70
#define N2K_DEVICE_CLASS_SENSOR_COMMUNICATION_INTERFACE 75
#define N2K_DEVICE_CLASS_INSTRUMENTATION_GENERAL_SYSTEMS 80
#define N2K_DEVICE_CLASS_EXTERNAL_ENVIRONMENT 85
#define N2K_DEVICE_CLASS_INTERNAL_ENVIRONMENT 90
#define N2K_DEVICE_CLASS_DECK_CARGO_FISHING_EQUIPMENT_SYSTEMS 100
#define N2K_DEVICE_CLASS_DISPLAY 120
#define N2K_DEVICE_CLASS_ENTERTAINMENT 125

#define N2K_DEVICE_FUNCTION_PC_GATEWAY  25_130
#define N2K_DEVICE_FUNCTION_PC_BATTERY  35_170
#define N2K_DEVICE_FUNCTION_PC_FLUID_LEVEL  75_150
#define N2K_DEVICE_FUNCTION_PC_GENERAL_PURPOSE_DISPLAYS 80_160
#define N2K_DEVICE_FUNCTION_PC_DISPLAY 120_130

#define N2K_INDUSTRY_CODE_GLOBAL 0
#define N2K_INDUSTRY_CODE_HIGHWAY 1
#define N2K_INDUSTRY_CODE_AGRICULTURE 2
#define N2K_INDUSTRY_CODE_CONSTRUCTION 3
#define N2K_INDUSTRY_CODE_MARINE 4
#define N2K_INDUSTRY_CODE_INDUSTRIAL 5

#define N2K_MANUFACTURER_CODE_UNKNOWN 2045

#define N2K_PRIORITY_0 0
#define N2K_PRIORITY_1 1
#define N2K_PRIORITY_2 2
#define N2K_PRIORITY_3 3
#define N2K_PRIORITY_4 4
#define N2K_PRIORITY_5 5
#define N2K_PRIORITY_6 6
#define N2K_PRIORITY_7 7
#define N2K_PRIORITY_LEAVE_UNCHANGED 8
#define N2K_PRIORITY_RESET_TO_DEFAULT 9

/*
 * PNGs
 * (+: implemented, -: not implemented yet)
 *
 * - 0x0EE00: PGN 60928 - ISO Address Claim
 * - 0x1F007: PGN 126983 - Alert
 * - 0x1F010: PGN 126992 - System Time
 * - 0x1F011: PGN 126993 - Heartbeat
 * - 0x1F014: PGN 126996 - Product Information
 * - 0x1F016: PGN 126998 - Configuration Information
 * - 0x1F105: PGN 127237 - Heading/Track control
 * - 0x1F10D: PGN 127245 - Rudder
 * - 0x1F112: PGN 127250 - Vessel Heading
 * - 0x1F113: PGN 127251 - Rate of Turn
 * - 0x1F114: PGN 127252 - Heave
 * + 0x1F211: PGN 127505 - Fluid Level
 * + 0x1F214: PGN 127508 - Battery Status
 * - 0x1F503: PGN 128259 - Speed
 * - 0x1F50B: PGN 128267 - Water Depth
 * - 0x1F513: PGN 128275 - Distance Log
 * - 0x1F805: PGN 129029 - GNSS Position Data
 * - 0x1F809: PGN 129033 - Time & Date
 * - 0x1FD02: PGN 130306 - Wind Data
 */

/*
 * 0x0EE00: PGN 60928 - ISO Address Claim
 */

#define N2K_PGN_ISO_ADDRESS_CLAIM 0x0EE00
#define N2K_PGN_ISO_ADDRESS_CLAIM_INTERVAL 0

typedef struct __attribute__((packed)) {
    unsigned int unique_number: 21; // ISO Identity Number
    unsigned int manufacturer_code: 11; // 0 .. 2045
    unsigned int device_instance_upper: 3; // ISO ECU Instance
    unsigned int device_instance_lower: 5; // 	ISO Function Instance
    unsigned int device_function: 8;
    unsigned int spare: 1;
    unsigned int device_class: 7;
    unsigned int system_instance: 4;
    unsigned int industry_group: 3;
    unsigned int reserved: 1;
} pgn_iso_address_claim_t;

static_assert( sizeof( pgn_iso_address_claim_t ) == 8, "Size of pgn_iso_address_claim_t is not correct" );

/*
 * 0x1F211: PGN 127505 - Fluid Level
 * transmitted every 2500 milliseconds
 */

#define N2K_PGN_FLUID_LEVEL 0x1F211
#define N2K_PGN_FLUID_LEVEL_INTERVAL 2500

typedef struct __attribute__((packed)) {
    unsigned int instance: 4;
    unsigned int type: 4;
    unsigned int level: 16;
    unsigned int capacity: 32;
    unsigned int reserved: 8;
} pgn_fluid_level_t;

static_assert( sizeof( pgn_fluid_level_t ) == 8, "Size of pgn_fluid_level_t is not correct" );

/*
 * 0x1F214: PGN 127508 - Battery Status
 * transmitted every 1500 milliseconds
 */

#define N2K_PGN_BATTERY_STATUS 0x1F214
#define N2K_PGN_BATTERY_STATUS_INTERVAL 5000 // 1500

typedef struct __attribute__((packed)) {
    unsigned int instance: 8;
    unsigned int voltage: 16; // 0.01V
    unsigned int current: 16; // 0.1A
    unsigned int temperature: 16; // 0.01K
    unsigned int sid: 8;
} pgn_battery_status_t;

static_assert( sizeof( pgn_battery_status_t ) == 8, "Size of pgn_battery_status_t is not correct" );
