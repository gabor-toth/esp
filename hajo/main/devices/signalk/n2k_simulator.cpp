#include <cmath>
#include "EspSigK.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "n2k_simulator.h"
#include "N2kMessages.h"
#include "N2kMsg.h"
#include "n2k_gateway.h"
#include "n2k/N2kRaymarine.h"

extern "C" {

static const char *TAG = "n2k-sim";

#define TICK_PERIOD_MS 10

static esp_timer_handle_t tick_timer = nullptr;

static int32_t currentTick = 0;
static unsigned char sid = 0;

static uint16_t DaysSince1970;
static double SecondsSinceMidnight;

static double SOG = KnotsToms( 5.2 ); // m/s
static double COG = DegToRad( 45.0 ); // rad
static double Latitude = 46.789117;
static double Longitude = 17.6058801;

static double WindSpeed = KnotsToms( 8.0 ); // m/s
static double WindAngle = COG + DegToRad( 45.0 ); // rad

static double BatteryVoltage1 = 12.8;
static double BatteryVoltage2 = 12.8;
static double BatteryVoltage3 = 12.8;
static double DepthBelowTransducer = 3.2;
static double FluidLevelFuel = 75.0;
static double FluidLevelWater = 65.0;

static void sendBatStatus( unsigned char BatteryInstance, double BatteryVoltage ) {
    tN2kMsg N2kMsg;
    SetN2kDCBatStatus( N2kMsg, BatteryInstance, BatteryVoltage );
    sendN2KMessageToSignalK( N2kMsg );
}

typedef enum {
    SendAttitude,
    SendBattery,
    SendCogSog,
    SendDepth,
    SendFluid,
    SendHeading,
    SendGNSS,
    SendLatLon,
    SendRudder,
    SendSpeed,
    SendWindSpeed,
    SendMax
} SenderType;

/*
static const char *SenderTypeNames[] = {
        "SendAttitude",
        "SendBattery",
        "SendCogSog",
        "SendDepth",
        "SendFluid",
        "SendHeading",
        "SendGNSS",
        "SendLatLon",
        "SendRudder",
        "SendSpeed",
        "SendWindSpeed",
        "SendMax",
};
*/

static uint32_t nextTickPerSenderType[SendMax] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint32_t nextTickForLog = 0;
static uint16_t sentPackets = 0;

static void logCount() {
    if ( nextTickForLog >= currentTick ) {
        return;
    }
    ESP_LOGI( TAG, "Sent %d packets", sentPackets );
    nextTickForLog = currentTick + pdMS_TO_TICKS( 1000 );
    sentPackets = 0;
}

static bool isTime( SenderType senderType, uint16_t milliseconds ) {
    uint32_t ticks = pdMS_TO_TICKS( milliseconds );
    uint32_t nextTick = nextTickPerSenderType[ senderType ];
    if ( nextTick >= currentTick ) {
        return false;
    }
    if ( nextTick == 0 ) {
        nextTick = currentTick;
    }
    nextTickPerSenderType[ senderType ] = nextTick + ticks;
    return true;
}

class Animator {
private:
    double &value;
    double deltaPerMs;
    uint16_t maxSteps;
    uint16_t currentStep;
public:
    Animator( double &value, double amplitude, uint16_t millisecondsPerCycle )
            : value( value ) {
        deltaPerMs = amplitude / ( (double)millisecondsPerCycle / TICK_PERIOD_MS );
        maxSteps = millisecondsPerCycle / TICK_PERIOD_MS;
        currentStep = maxSteps / 2;
    };

    void animate() {
        value += deltaPerMs;
        if ( --currentStep == 0 ) {
            currentStep = maxSteps;
            deltaPerMs = -deltaPerMs;
        }
    }

};

static Animator animators[] = {
//        "SendAttitude",
//        "SendBattery",
        Animator(BatteryVoltage1,0.5, 15000),
        Animator(BatteryVoltage2,1.0, 10000),
        Animator(BatteryVoltage3,1.5, 10000),
//        "SendCogSog",
//        "SendDepth",
        Animator(DepthBelowTransducer,1.0, 15000),
//        "SendFluid",
        Animator(FluidLevelFuel,10.0, 20000),
        Animator(FluidLevelWater,10.0, 15000),
//        "SendHeading",
//        "SendGNSS",
//        "SendLatLon",
//        "SendRudder",
//        "SendSpeed",
//        "SendWindSpeed",
        Animator(WindSpeed,KnotsToms(4.0), 60000),
        Animator(WindAngle,DegToRad(20.0), 30000),
//        "SendMax",
};

static void animate() {
    for( auto &animator: animators) {
        animator.animate();
    }
}
static void onSimulatorTick( void *arg ) {
    sid++;

    // animation
    SecondsSinceMidnight += TICK_PERIOD_MS / 1000.0;
    currentTick++;
    logCount();
    animate();

    if ( isTime( SendBattery, 1500 ) ) {
        // electrical.batteries.[1,2,3].voltage, 1500ms
        {
            tN2kMsg N2kMsg;
            SetN2kDCBatStatus( N2kMsg, 1, BatteryVoltage1 );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        {
            tN2kMsg N2kMsg;
            SetN2kDCBatStatus( N2kMsg, 2, BatteryVoltage2 );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        {
            tN2kMsg N2kMsg;
            SetN2kDCBatStatus( N2kMsg, 3, BatteryVoltage3 );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendDepth, 1500 ) ) {
        {
            tN2kMsg N2kMsg;
            // environment.depth.*, 1000ms
            SetN2kWaterDepth( N2kMsg, sid, DepthBelowTransducer, -1.8 );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendWindSpeed, 100 ) ) {
        {
            // environment.wind.angleApparent, environment.wind.speedApparent, 100ms
            tN2kMsg N2kMsg;
            SetN2kWindSpeed( N2kMsg, sid, WindSpeed, WindAngle, N2kWind_Apparent );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        // Raymarine device does not send these, they are derived using speedThroughWater
//        {
//            // environment.wind.angleTrueGround, environment.wind.speedTrue, 100ms
//            tN2kMsg N2kMsg;
//            SetN2kWindSpeed( N2kMsg, sid, WindSpeed, WindAngle, N2kWind_True_boat );
//            sendN2KMessageToSignalK( N2kMsg );
//            sentPackets++;
//        }
//        {
//            // environment.wind.angleTrueWater, environment.wind.speedTrue, 100ms
//            tN2kMsg N2kMsg;
//            SetN2kWindSpeed( N2kMsg, sid, WindSpeed, WindAngle, N2kWind_True_water );
//            sendN2KMessageToSignalK( N2kMsg );
//            sentPackets++;
//        }
        return;
    }

    if ( isTime( SendAttitude, 1000 ) ) {
        // navigation.attitude.*, 1000ms
        tN2kMsg N2kMsg;
        static int pitch = 3;
        SetN2kAttitude( N2kMsg, 0, 0.0, pitch / 180.0, 0.0 );
        sentPackets++;
        return;
    }

    if ( isTime( SendCogSog, 250 ) ) {
        {
            tN2kMsg N2kMsg;
            // navigation.courseOverGroundTrue, navigation.speedOverGround, 250ms
            SetN2kCOGSOGRapid( N2kMsg, sid, N2khr_true, COG, SOG );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    // navigation.courseGreatCircle.nextPoint.bearingTrue, ?ms
    // ?

    // navigation.gnss.*, navigation.position, 1000ms

    if ( isTime( SendGNSS, 1000 ) ) {
        {
            tN2kMsg N2kMsg;
            SetN2kGNSS( N2kMsg, sid, DaysSince1970, SecondsSinceMidnight,
                        Latitude, Longitude, N2kDoubleNA,
                        N2kGNSSt_GPS, N2kGNSSm_GNSSfix,  N2kGNSSi_noIntegrityChecking,
                        7, 2, N2kDoubleNA, N2kDoubleNA,
                        1, N2kGNSSt_GPS, 0, 0 );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendHeading, 100 ) ) {
        {
            // navigation.headingMagnetic, 100ms
            tN2kMsg N2kMsg;
            SetN2kPGN65359( N2kMsg, sid, N2kDoubleNA, COG + DegToRad(6.0));
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
//        {
        // TODO navigation.headingMagnetic 	n2k.104 (autopilot)  (65359)
//        {
//            // navigation.headingMagnetic, 100ms
//            tN2kMsg N2kMsg;
//            SetN2kMagneticHeading( N2kMsg, sid, COG + DegToRad(6.0));
//            sendN2KMessageToSignalK( N2kMsg );
//            sentPackets++;
//        }
//            // navigation.headingMagnetic, 100ms
//            tN2kMsg N2kMsg;
//            SetN2kTrueHeading( N2kMsg, sid, COG );
//            sendN2KMessageToSignalK( N2kMsg );
//            sentPackets++;
//        }
        return;
    }

    if ( isTime( SendLatLon, 100 ) ) {
        {
            // navigation.position", 100ms
            tN2kMsg N2kMsg;
            SetN2kLatLonRapid( N2kMsg, Latitude, Longitude );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendSpeed, 1000 ) ) {
        {
            // navigation.speedThroughWater, 1000ms
            tN2kMsg N2kMsg;
            SetN2kBoatSpeed( N2kMsg, sid, SOG, N2kDoubleNA, N2kSWRT_Paddle_wheel );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendRudder, 100 ) ) {
        {
            // steering.rudderAngle, 100ms
            static double RudderPosition = 3.0;
            tN2kMsg N2kMsg;
            SetN2kRudder( N2kMsg, DegToRad( RudderPosition ) );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    if ( isTime( SendFluid, 2500 ) ) {
        // tanks.[freshWater,fuel].1.currentLevel, 2500ms
        {
            tN2kMsg N2kMsg;
            SetN2kFluidLevel( N2kMsg, 0, N2kft_Fuel, FluidLevelFuel, N2kDoubleNA );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        {
            tN2kMsg N2kMsg;
            SetN2kFluidLevel( N2kMsg, 0, N2kft_Water, FluidLevelWater, N2kDoubleNA );
            sendN2KMessageToSignalK( N2kMsg );
            sentPackets++;
        }
        return;
    }

    // steering.autopilot.state 	n2k.104  (126720)
    // tanks.fuel.0.currentLevel 	0.0006  ratio	07/28 12:47:24	n2k.29  (127505)
    // tanks.freshWater.0.currentLevel 	0.005   ratio	07/28 12:47:24	n2k.29  (127505)
}

void pngSimulationStart() {
    ESP_LOGI( TAG, "png simulation start" );
    if ( tick_timer == nullptr ) {
        esp_timer_create_args_t timer_args = {
                .callback = onSimulatorTick,
                .arg = nullptr,
                .dispatch_method = ESP_TIMER_TASK,
                .name = nullptr
        };
        ESP_ERROR_CHECK( esp_timer_create( &timer_args, &tick_timer ) );
    }
    ESP_ERROR_CHECK( esp_timer_start_periodic( tick_timer, TICK_PERIOD_MS * 1000 ) );
}

void pngSimulationStop() {
    ESP_LOGI( TAG, "png simulation stop" );
    esp_timer_stop( tick_timer );
}

void pngSimulationOneOff() {
#if CONFIG_SIGNALK_OVER_WIFI
    ESP_LOGI( TAG, "png simulation one off" );

    DeltaSet deltaSet( 0, 0 );
    deltaSet.addValue( "electrical.batteries.1.voltage", 11.6 );
    deltaSet.addValue( "electrical.batteries.2.voltage", 11.6 );
    deltaSet.addValue( "electrical.batteries.3.voltage", 11.6 );
    deltaSet.addValue( "environment.depth.belowKeel", 0.1 );
    deltaSet.addValue( "environment.depth.belowSurface", 0.1 );
    deltaSet.addValue( "environment.depth.belowTransducer", 0.1 );
    deltaSet.addValue( "environment.wind.angleApparent", 0.1 );
    deltaSet.addValue( "environment.wind.angleTrueGround", 0.1 );
    deltaSet.addValue( "environment.wind.angleTrueWater", 0.1 );
    deltaSet.addValue( "environment.wind.speedApparent", 0.1 );
    deltaSet.addValue( "environment.wind.speedTrue", 0.1 );
    deltaSet.addValue( "navigation.attitude.pitch", 0.1 );
    deltaSet.addValue( "navigation.attitude.roll", 0.1 );
    deltaSet.addValue( "navigation.courseOverGroundTrue", 0.1 );
    deltaSet.addValue( "navigation.courseGreatCircle.nextPoint.bearingTrue", 0.1 );
    deltaSet.addValue( "navigation.datetime", "2024-06-11T00:52:00Z" );
    deltaSet.addValue( "navigation.headingTrue", 0 );
    deltaSet.addJsonValue( "navigation.position", R"({"altitude":0,"latitude":46.789117,"longitude":17.6058801})" );
    deltaSet.addValue( "navigation.speedOverGround", 2.6 );
    deltaSet.addValue( "navigation.speedThroughWater", 2.5 );
    deltaSet.addValue( "steering.rudderAngle", 0.1 );
    deltaSet.addValue( "tanks.freshWater.1.currentLevel", 0.25 );
    deltaSet.addValue( "tanks.fuel.1.currentLevel", 0.1 );
    deltaSet.send( sigK );
#endif
}

}
