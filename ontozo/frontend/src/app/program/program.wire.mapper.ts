import { Injectable } from "@angular/core";
import { ProgramDayWire, ProgramWire } from "./program.wire";
import { Program, ProgramDay, ProgramDayType, ProgramDayValue } from "./program";

@Injectable( {
  providedIn: 'root'
} )
export class ProgramWireMapper {
  public mapProgramFromWire( programWire: ProgramWire ) {
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
      index: programWire.index,
      lastRunTime: programWire.lastRunTime,
      name: programWire.name,
      startTimes: programWire.startTimes,
      nextRunTime: programWire.nextRunTime,
      zones: programWire.zones,
    };
  }

  public mapProgramsFromWire( programs: ProgramWire[] ) {
    let service = this;
    return programs.map( ( program ) => {
      return service.mapProgramFromWire( program );
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
      index: program.index,
      lastRunTime: program.lastRunTime,
      name: program.name,
      startTimes: program.startTimes,
      nextRunTime: program.nextRunTime,
      zones: program.zones,
    };
  }

  private mapDayToWire( value: ProgramDayValue ) {
    return ProgramDayValue[ value ];
  }
}
