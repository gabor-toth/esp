export interface RunZoneState {
  enabled: boolean;
  duration: number;
  id: number;
  leftSeconds: number;
  name: string;
  running: boolean;
}

export interface RunProgramState {
  duration: number;
  id: number;
  name: string;
  running: boolean;
  zones: RunZoneState[];
}

export interface RunState {
  programs: RunProgramState[];
  isProgramRunning: boolean;
}

