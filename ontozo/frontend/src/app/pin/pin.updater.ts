import { Injectable } from '@angular/core';
import { PinsState } from './pin';
import { PinService } from "./pin.service";
import { TimedUpdater } from "../common/timed.updater";

@Injectable( {
  providedIn: 'root'
} )
export class PinUpdater extends TimedUpdater<PinsState> {
  constructor( private pinService: PinService ) {
    super();
  }

  protected override getState() {
    return this.pinService.getState();
  }

}
