import { Injectable } from '@angular/core';
import { PinConfiguration, PinsConfiguration, PinsState, PinState } from './pin';
import { Observable, of, Subscriber } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { simulatedPinConfiguration, simulatedPinState } from '../simulator/simulator';

@Injectable( {
  providedIn: 'root'
} )
export class PinStateAmender {

  constructor() {
  }

  process( configuration: PinsConfiguration, state: PinsState ): PinsState {
    this.copyConfiguration( configuration.inputs.buttons, state.inputs.buttons );
    this.copyConfiguration( configuration.inputs.levels, state.inputs.levels );
    this.copyConfiguration( configuration.outputs.pumps, state.outputs.pumps );
    this.copyConfiguration( configuration.outputs.zones, state.outputs.zones );
    return state;
  }

  private copyConfiguration( configuration: PinConfiguration[], state: PinState[] ) {
    for ( let i = 0; i < configuration.length; i++ ) {
      state[ i ].name = configuration[ i ].name;
    }
  }
}
