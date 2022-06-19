export interface PinState {
    id: number;
    name: string;
    on: boolean;
}

export interface InputsState {
    levels: PinState[];
    buttons: PinState[];
}

export interface OutputsState {
    zones: PinState[];
    pumps: PinState[];
}

export interface TimeState {
    time?: String;
    isTimeSet: boolean;
}

export interface State {
    inputs: InputsState;
    outputs: OutputsState;
    time?: TimeState | null;
}
