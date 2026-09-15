/*
 * configuration
 */

export interface PinConfiguration {
  hidden: boolean;
  id: number;
  inactive: boolean;
  name: string;
  manual: boolean;
}

export interface PinInputsConfiguration {
  buttons: PinConfiguration[];
  levels: PinConfiguration[];
}

export interface PinOutputsConfiguration {
  pumps: PinConfiguration[];
  zones: PinConfiguration[];

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
  buttons: PinState[];
  levels: PinState[];
}

export interface PinOutputsState {
  pumps: PinState[];
  zones: PinState[];
}

export interface TimeState {
  isTimeSet: boolean;
  time?: string;
}

export enum LevelState {
  failure = -1,
  empty,
  filling,
  full
}

export interface PinsState {
  inputs: PinInputsState;
  levels: LevelState[];
  outputs: PinOutputsState;
  time?: TimeState | undefined;
  version: string;
}

export class PinHelper {

  static getPinById( pins: PinConfiguration[] | undefined, id: number ): PinConfiguration | undefined {
    return pins == undefined ? undefined : pins.find( pin => pin.id === id );
  }
}