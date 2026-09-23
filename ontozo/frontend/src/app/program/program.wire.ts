import { ProgramDayType, ProgramDayValue, ProgramZone } from "./program";

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
  nextRunTime?: number;
  index: number;
  lastRunTime?: number;
  name: string;
  startTimes: string[];
  zones: ProgramZone[];
}

export interface ProgramsWire {
  programs: ProgramWire[];
  version: string;
}

