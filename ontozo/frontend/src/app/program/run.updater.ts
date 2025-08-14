import { Injectable } from '@angular/core';
import { RunState } from "./run";
import { RunService } from "./run.service";
import { TimedUpdater } from "../common/timed.updater";

@Injectable( {
  providedIn: 'root'
} )
export class RunUpdater extends TimedUpdater<RunState> {
  constructor( private runService: RunService ) {
    super();
  }

  protected override getState() {
    return this.runService.getState();
  }

}
