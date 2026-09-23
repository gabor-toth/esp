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
  onDays?: ProgramDayValue[];
  intervalDays?: number;
  intervalStartsOn?: number;
  intervalStartReset?: boolean;
}

export interface Program {
  days: ProgramDay;
  enabled: boolean;
  index: number;
  lastRunTime?: number;
  name: string;
  nextRunTime?: number;
  startTimes: string[];
  zones: ProgramZone[];
}
