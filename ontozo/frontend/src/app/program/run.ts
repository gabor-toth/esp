export interface RunZoneState {
  disabled: boolean;
  duration: number;
  index: number;
  leftSeconds: number;
  name: string;
  running: boolean;
}

export interface RunProgramState {
  duration: number;
  id: number;
  index: number;
  name: string;
  running: boolean;
  zones: RunZoneState[];
}

export interface RunState {
  programs: RunProgramState[];
  isProgramRunning: boolean;
}

