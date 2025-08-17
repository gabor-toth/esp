import { Component, OnInit } from '@angular/core';
import { PinsState } from "../pin/pin";
import { PinService } from "../pin/pin.service";
import { animate, state, style, transition, trigger } from "@angular/animations";
import { RunService } from "../program/run.service";
import { RunProgramState, RunState } from "../program/run";
import { PinUpdater } from "../pin/pin.updater";
import { Subscription } from "rxjs";
import { RunUpdater } from "../program/run.updater";

@Component( {
  selector: 'app-state',
  templateUrl: './state.component.html',
  styleUrls: [ './state.component.scss' ],
  animations: [
    trigger( 'detailExpand', [
      state( 'collapsed', style( { height: '0px', minHeight: '0' } ) ),
      state( 'expanded', style( { height: '*' } ) ),
      transition( 'expanded <=> collapsed', animate( '225ms cubic-bezier(0.4, 0.0, 0.2, 1)' ) ),
    ] ),
  ],
} )
export class StateComponent implements OnInit {
  pinState: PinsState | undefined;
  runState: RunProgramState | undefined;
  queueState: RunProgramState[] | undefined;
  remoteTime: String | undefined;
  pinStateAsString = "";
  programStateAsString = "";
  pinUpdaterSubscription?: Subscription;
  runUpdaterSubscription?: Subscription;

  constructor( private pinService: PinService,
               private pinUpdater: PinUpdater,
               private runService: RunService,
               private runUpdater: RunUpdater ) {
  }

  ngOnInit(): void {
    this.subscribeForUpdates();
  }

  ngOnDestroy(): void {
    this.pinUpdaterSubscription?.unsubscribe();
    this.runUpdaterSubscription?.unsubscribe();
  }

  private subscribeForUpdates() {
    let component = this;
    this.pinUpdaterSubscription = this.pinUpdater.subscribe( {
      next( state ) {
        component.onUpdatePinState( state );
      },
    } );
    this.runUpdaterSubscription = this.runUpdater.subscribe( {
      next( state ) {
        component.onUpdateRunState( state );
      },
    } );
  }

  private onUpdatePinState( newState: PinsState ) {
    this.remoteTime = newState.time?.time;
    newState.time = null;
    let newStateAsString = JSON.stringify( newState );
    if ( newStateAsString != this.pinStateAsString ) {
      this.pinState = newState;
      this.pinStateAsString = newStateAsString;
    }
  }

  private onUpdateRunState( newState: RunState ) {
    let newStateAsString = JSON.stringify( newState );
    if ( newStateAsString != this.programStateAsString ) {
      if ( newState.isProgramRunning ) {
        this.runState = newState.programs[ 0 ];
        this.runState.running = true;
        for ( let zoneIndex = 0; this.runState.zones.length; zoneIndex++ ) {
          let zone = this.runState.zones[ zoneIndex ];
          if ( zone.running ) {
            zone.duration = zone.leftSeconds;
            if ( zoneIndex > 0 ) {
              this.runState.zones = this.runState.zones.slice( zoneIndex );
            }
            break;
          }
        }
        this.calculateDuration( this.runState );
        this.queueState = newState.programs.slice( 1 );
        this.queueState.forEach( ( p ) => this.calculateDuration( p ) );
      } else {
        this.runState = <RunProgramState>{};
        this.queueState = undefined;
      }
      this.programStateAsString = newStateAsString;
    }
  }

  private calculateDuration( program: RunProgramState ) {
    let duration = 0;
    program.zones.forEach( ( zone ) => duration += zone.duration );
    program.duration = duration;
  }

  click( type: String, id: number, state: boolean ) {
    let component = this;
    this.pinService.setState( type, id, state ).subscribe( {
      complete() {
        component.pinUpdater.updateState();
        component.runUpdater.updateState();
      },
      error( err ) {
        console.error( 'Error writing state', err );
      }
    } );
  }
}
