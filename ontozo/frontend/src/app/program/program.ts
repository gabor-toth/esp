export interface ProgramZone {
    zoneId: number;
    duration: number;
}

export enum ProgramDayType {
    unused,
    on,
    interval
}

export interface ProgramDay {
    type: ProgramDayType;
    onDays: string[];
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

export interface ProgramHeader {
    index: number;
    enabled: boolean;
    name: string;
    program: Program;
}
