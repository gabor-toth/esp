#include "n2k_struct_parser.h"
#include "N2kMessages.h"

bool ParseN2kFluidLevel( const tN2kMsg &N2kMsg, N2kFluidLevelData &data ) {
    return ParseN2kFluidLevel( N2kMsg,
                               data.instance,
                               data.fluidType,
                               data.level,
                               data.capacity );
}


bool ParseN2kDCStatus( const tN2kMsg &N2kMsg, ParseN2kDCStatusData &data ) {
    return ParseN2kDCStatus( N2kMsg,
                             data.sid,
                             data.instance,
                             data.dcType,
                             data.stateOfCharge,
                             data.stateOfHealth,
                             data.timeRemaining,
                             data.rippleVoltage,
                             data.capacity );
}


bool ParseN2kDCBatStatus( const tN2kMsg &N2kMsg, N2kDCBatStatusData &data ) {
    return ParseN2kDCBatStatus( N2kMsg,
                                data.instance,
                                data.voltage,
                                data.current,
                                data.temperature,
                                data.sid );
}

bool ParseN2kBatConf( const tN2kMsg &N2kMsg, N2kBatConfData &data ) {
    return ParseN2kBatConf( N2kMsg,
                            data.instance,
                            data.batType,
                            data.supportsEqual,
                            data.batNominalVoltage,
                            data.batChemistry,
                            data.batCapacity,
                            data.batTemperatureCoefficient,
                            data.peukertExponent,
                            data.chargeEfficiencyFactor );
}

bool ParseN2kGNSS( const tN2kMsg &N2kMsg, N2kGNSSData &data ) {
    return ParseN2kGNSS( N2kMsg,
                         data.sid,
                         data.daysSince1970,
                         data.secondsSinceMidnight,
                         data.latitude,
                         data.longitude,
                         data.altitude,
                         data.gnssType,
                         data.gnssMethod,
                         data.satellites,
                         data.hdop,
                         data.pdop,
                         data.geoidalSeparation,
                         data.referenceStations,
                         data.referenceStationType,
                         data.referenceStationID,
                         data.ageOfCorrection );
}

bool ParseN2kLocalOffset( const tN2kMsg &N2kMsg, N2kLocalOffsetData &data ) {
    return ParseN2kLocalOffset( N2kMsg,
                                data.daysSince1970,
                                data.secondsSinceMidnight,
                                data.localOffset );
}

bool ParseN2kRudder( const tN2kMsg &N2kMsg, N2kRudderData &data ) {
    return ParseN2kRudder( N2kMsg,
                           data.rudderPosition,
                           data.instance,
                           data.rudderDirectionOrder,
                           data.angleOrder );
}

bool ParseN2kAttitude( const tN2kMsg &N2kMsg, N2kAttitudeData &data ) {
    return ParseN2kAttitude( N2kMsg,
                             data.instance,
                             data.yaw,
                             data.pitch,
                             data.roll );
}
