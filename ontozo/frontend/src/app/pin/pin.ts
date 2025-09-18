/*
 * configuration
 */

export interface PinConfiguration {
  id: number;
  name: string;
  manual: boolean;
}

export interface PinInputsConfiguration {
  levels: PinConfiguration[];
  buttons: PinConfiguration[];
}

export interface PinOutputsConfiguration {
  zones: PinConfiguration[];
  pumps: PinConfiguration[];
}

export interface PinsConfiguration {
  inputs: PinInputsConfiguration;
  outputs: PinOutputsConfiguration;
  version: string;
}

/*
 * state
 */

export interface PinState {
  id: number;
  name: string; // filled from PinConfiguration.name
  on: boolean;
}

export interface PinInputsState {
  levels: PinState[];
  buttons: PinState[];
}

export interface PinOutputsState {
  zones: PinState[];
  pumps: PinState[];
}

export interface TimeState {
  time?: string;
  isTimeSet: boolean;
}

export interface PinsState {
  inputs: PinInputsState;
  outputs: PinOutputsState;
  version: string;
  time?: TimeState | undefined;
}
