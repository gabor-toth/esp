export interface ProgramZone {
  zoneId: number;
  duration: number;
}

export enum ProgramDayType {
  unused,
  onDays,
  interval
}

export enum ProgramDayValue {
  Mon,
  Tue,
  Wed,
  Thu,
  Fri,
  Sat,
  Sun,
}

export interface ProgramDay {
  type: ProgramDayType;
  onDays: ProgramDayValue[];
  intervalDays: number;
  intervalStartsOn: number;
  intervalStartReset: boolean;
}

export interface ProgramShort {
  index: number;
  name: string;
}

export interface Program extends ProgramShort {
  days: ProgramDay;
  enabled: boolean;
  nextRunTime: number;
  lastRunTime: number;
  startTimes: string[];
  valid: boolean;
  zones: ProgramZone[];
}
