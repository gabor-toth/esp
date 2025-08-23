export interface PinState {
  id: number;
  name: string;
  on: boolean;
}

export interface PinInputsState {
  levels: PinState[];
  buttons: PinState[] ;
}

export interface PinOutputsState {
  zones: PinState[];
  pumps: PinState[];
}

export interface TimeState {
  time?: String;
  isTimeSet: boolean;
}

export interface PinsState {
  inputs: PinInputsState;
  outputs: PinOutputsState;
  time?: TimeState | null;
}
