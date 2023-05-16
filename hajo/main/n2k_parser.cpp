#include "n2k_parser.h"
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
                         data.SID,
                         data.DaysSince1970,
                         data.SecondsSinceMidnight,
                         data.Latitude,
                         data.Longitude,
                         data.Altitude,
                         data.GNSStype,
                         data.GNSSmethod,
                         data.nSatellites,
                         data.HDOP,
                         data.PDOP,
                         data.GeoidalSeparation,
                         data.nReferenceStations,
                         data.ReferenceStationType,
                         data.ReferenceStationID,
                         data.AgeOfCorrection );
}

bool ParseN2kLocalOffset( const tN2kMsg &N2kMsg, N2kLocalOffsetData &data ) {
    return ParseN2kLocalOffset( N2kMsg,
                                data.DaysSince1970,
                                data.SecondsSinceMidnight,
                                data.LocalOffset );
}