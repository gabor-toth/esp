export interface ProgramZoneWire {
  id: number;
  duration: number; // in seconds
}

export interface ProgramDayWire {
  type: string;
  onDays?: string[];
  intervalDays?: number;
  intervalStartsOn?: string;
  intervalStartReset?: boolean;
}

export interface ProgramWire {
  days: ProgramDayWire;
  enabled: boolean;
  id: number;
  lastRunTime?: number;
  name: string;
  nextRunTime?: number;
  startTimes: string[];
  zones: ProgramZoneWire[];
}

export interface ProgramsWire {
  programs: ProgramWire[];
  version: string;
}

