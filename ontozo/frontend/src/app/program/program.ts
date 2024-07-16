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

export interface Program {
  index: number;
  valid: boolean;
  enabled: boolean;
  name: string;
  zones: ProgramZone[];
  startTimes: string[];
  days: ProgramDay;
  lastRunTime: number;
  nextRunTime: number;
}
