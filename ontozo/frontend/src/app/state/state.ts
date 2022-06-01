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

export interface State {
    inputs: InputsState;
    outputs: OutputsState;
}
