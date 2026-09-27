import { Injectable } from "@angular/core";
import { ProgramDayWire, ProgramWire, ProgramZoneWire } from "./program.wire";
import { Program, ProgramDay, ProgramDayType, ProgramDayValue, ProgramZone } from "./program";
import { PinsConfiguration } from "../pin/pin";

@Injectable( {
  providedIn: 'root'
} )
export class ProgramWireMapper {
  public mapProgramFromWire( programWire: ProgramWire, pinState: PinsConfiguration ) {
    let service = this;
    let days = <ProgramDay>{
      type: ProgramDayType[ programWire.days.type as keyof typeof ProgramDayType ],
    };
    if ( days.type == ProgramDayType.onDays ) {
      days.onDays = programWire.days.onDays?.map( value => {
        return service.mapDayFromWire( value );
      } );
    } else if ( days.type == ProgramDayType.interval ) {
      days.intervalDays = programWire.days.intervalDays;
      if ( programWire.days.intervalStartsOn != undefined ) {
        days.intervalStartsOn = service.mapDayFromWire( programWire.days.intervalStartsOn );
      }
      days.intervalStartReset = programWire.days.intervalStartReset;
    }
    return <Program>{
      days: days,
      enabled: programWire.enabled,
      id: programWire.id,
      lastRunTime: programWire.lastRunTime,
      name: programWire.name,
      startTimes: programWire.startTimes,
      nextRunTime: programWire.nextRunTime,
      zones: programWire.zones.map( zone => service.mapZoneFromWire( zone, pinState ) ),
    };
  }

  public mapZoneFromWire( programZone: ProgramZoneWire, pinState: PinsConfiguration ) {
    let zoneConfig = pinState.outputs.zones.find( zone => zone.id == programZone.id );
    return {
      duration: programZone.duration / 60,
      id: programZone.id,
      name: zoneConfig?.name != undefined ? zoneConfig.name : programZone.id.toString(),
    };
  }

  public mapProgramsFromWire( programs: ProgramWire[], pinState: PinsConfiguration ) {
    let service = this;
    return programs.map( ( program ) => {
      return service.mapProgramFromWire( program, pinState );
    } );
  }

  private mapDayFromWire( value: string ) {
    return ProgramDayValue[ value as keyof typeof ProgramDayValue ];
  }

  public mapProgramToWire( program: Program ) {
    let service = this;
    let days = <ProgramDayWire>{
      type: ProgramDayType[ program.days.type ].toString(),
    };
    if ( program.days.type == ProgramDayType.onDays ) {
      days.onDays = program.days.onDays?.map( value => {
        return service.mapDayToWire( value );
      } );
    } else if ( program.days.type == ProgramDayType.interval ) {
      days.intervalDays = program.days.intervalDays;
      if ( program.days.intervalStartsOn != undefined ) {
        days.intervalStartsOn = service.mapDayToWire( program.days.intervalStartsOn );
      }
      days.intervalStartReset = program.days.intervalStartReset;
    }
    return <ProgramWire>{
      days: days,
      enabled: program.enabled,
      id: program.id,
      lastRunTime: program.lastRunTime,
      name: program.name,
      nextRunTime: program.nextRunTime,
      startTimes: program.startTimes,
      zones: program.zones.map( zone => service.mapZoneToWire( zone ) ),
    };
  }

  public mapZoneToWire( programZone: ProgramZone ) {
    return {
      duration: programZone.duration * 60,
      id: programZone.id,
    };
  }

  private mapDayToWire( value: ProgramDayValue ) {
    return ProgramDayValue[ value ];
  }
}
