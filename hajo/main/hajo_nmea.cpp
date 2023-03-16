#include "hajo_nmea.h"
#include "NMEA2000_CAN.h"       // This will automatically choose right CAN library and create suitable NMEA2000 object
#include "N2kMessages.h"
#include "NMEA2000_esp32_timer.h"

#include "esp_log.h"

const char *LOG = "main";

// List here messages your device will transmit.
const unsigned long TransmitMessages[] PROGMEM = { 127506L, 127508L, 127513L, 0 };

// ---  Example of using PROGMEM to hold Product ID.  However, doing this will prevent any updating of
//      these details except of recompiling the program.
const tNMEA2000::tProductInformation BatteryMonitorProductInformation PROGMEM = {
        2100,                        // N2kVersion
        100,                         // Manufacturer's product code
        "Simple battery monitor",    // Manufacturer's Model ID
        "1.2.0.16 (2022-10-01)",     // Manufacturer's Software version code
        "1.2.0.0 (2022-10-01)",      // Manufacturer's Model version
        "00000001",                  // Manufacturer's Model serial code
        0,                           // SertificationLevel
        1                            // LoadEquivalency
};

// ---  Example of using PROGMEM to hold Configuration information.  However, doing this will prevent any updating of
//      these details except of recompiling the program.
const char BatteryMonitorManufacturerInformation[] PROGMEM = "John Doe, john.doe@unknown.com";
const char BatteryMonitorInstallationDescription1[] PROGMEM = "Just for sample";
const char BatteryMonitorInstallationDescription2[] PROGMEM = "No real information send to bus";

// Define schedulers for messages. Define schedulers here disabled. Schedulers will be enabled
// on OnN2kOpen, so they will be synchronized with system.
// We use own scheduler for each message so that each can have different offset and period.
// Setup periods according PGN definition (see comments on IsDefaultSingleFrameMessage and
// IsDefaultFastPacketMessage) and message first start offsets. Use a bit of different offset for
// each message, so they will not be sent at same time.

static bool prepareDCBatStatus( int index, tN2kMsg &N2kMsg ) {
    if ( index > 0 ) {
        return false;
    }
    SetN2kDCBatStatus( N2kMsg, 1, 13.87, 5.12, 35.12, 1 );
    return true;
}

static bool prepareDCStatus( int index, tN2kMsg &N2kMsg ) {
    if ( index > 0 ) {
        return false;
    }
    SetN2kDCStatus( N2kMsg, 1, 1, N2kDCt_Battery, 56, 92, 38500, 0.012 );
    return true;
}

static bool prepareBatConf( int index, tN2kMsg &N2kMsg ) {
    if ( index > 0 ) {
        return false;
    }
    SetN2kBatConf( N2kMsg, 1, N2kDCbt_Gel, N2kDCES_Yes, N2kDCbnv_12v, N2kDCbc_LeadAcid, AhToCoulomb( 420 ), 53,
                   1.251, 75 );
    return true;
}

EspN2kTimer DCBatStatusScheduler( "dcbatstat", 1500, prepareDCBatStatus );
EspN2kTimer DCStatusScheduler( "dcstatus", 1500, prepareDCStatus );
EspN2kTimer BatConfScheduler( "batconf", 5000, prepareBatConf );

void OnN2kOpen() {
    ESP_LOGI( LOG, "OnN2kOpen" );
    DCBatStatusScheduler.start();
    DCStatusScheduler.start();
    BatConfScheduler.start();
}

void setup() {
    // Set Product information
    NMEA2000.SetProductInformation( &BatteryMonitorProductInformation );
    // Set Configuration information
    NMEA2000.SetProgmemConfigurationInformation( BatteryMonitorManufacturerInformation,
                                                 BatteryMonitorInstallationDescription1,
                                                 BatteryMonitorInstallationDescription2 );
    // Set device information
    NMEA2000.SetDeviceInformation( 1,      // Unique number. Use e.g. Serial number.
                                   170,    // Device function=Battery. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   35,     // Device class=Electrical Generation. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                   2046    // Just chosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );


    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
    NMEA2000.SetMode( tNMEA2000::N2km_ListenAndNode, 25 );
    // NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText);     // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
    // NMEA2000.EnableForward(false);                      // Disable all msg forwarding to USB (=Serial)

    //  NMEA2000.SetN2kCANMsgBufSize(2);                    // For this simple example, limit buffer size to 2, since we are only sending data
    // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
    NMEA2000.SetOnOpen( OnN2kOpen );
    NMEA2000.Open();
}

extern "C" {
void app_main() {
    setup();
}
}
