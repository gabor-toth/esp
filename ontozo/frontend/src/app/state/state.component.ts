import { Component, OnInit } from '@angular/core';
import { PinsConfiguration, PinsState } from "../pin/pin";
import { PinService } from "../pin/pin.service";
import { animate, state, style, transition, trigger } from "@angular/animations";
import { RunService } from "../program/run.service";
import { RunProgramState, RunState, RunZoneState } from "../program/run";
import { PinUpdater } from "../pin/pin.updater";
import { Subscription } from "rxjs";
import { RunUpdater } from "../program/run.updater";
import { ProgramService } from "../program/program.service";
import { MatSnackBar } from "@angular/material/snack-bar";
import { SnackbarErrorComponent } from "../common/snackbar-error/snackbar-error.component";
import { DatePipe } from '@angular/common';
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatFormField } from '@angular/material/form-field';
import { MatOption, MatSelect } from '@angular/material/select';
import { MatButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";
import { Program } from "../program/program";

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
  imports: [
    DatePipe,
    MatIcon,
    MatProgressSpinner,
    MatFormField,
    MatSelect,
    MatOption,
    MatButton,
    MatCard,
    MatCardHeader,
    MatCardContent,
    MatCardTitle,
  ]
} )
export class StateComponent implements OnInit {
  pins: PinsConfiguration | undefined;
  pinState: PinsState | undefined;
  pinStateAsString = "";
  pinUpdaterSubscription?: Subscription;
  programStateAsString = "";
  queueState: RunProgramState[] | undefined;
  programs: Program[] | undefined;
  remoteTime: string | undefined;
  runState: RunProgramState | undefined;
  runUpdaterSubscription?: Subscription;
  selectedProgramIndex: number = 0;

  constructor( private snackBar: MatSnackBar,
               private pinService: PinService,
               private pinUpdater: PinUpdater,
               private programService: ProgramService,
               private runService: RunService,
               private runUpdater: RunUpdater ) {
  }

  ngOnInit(): void {
    this.subscribeForUpdates();
    this.loadPrograms();
  }

  ngOnDestroy(): void {
    this.pinUpdaterSubscription?.unsubscribe();
    this.runUpdaterSubscription?.unsubscribe();
  }

  private loadPrograms() {
    let component = this;
    this.programService.getAll().subscribe( {
      next( state ) {
        component.programs = state;
      },
      error( error ) {
        component.openSnackBar( 'Error loading programs', error );
      }
    } );
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
    newState.time = undefined;
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
        this.queueState = [];
      }
      this.programStateAsString = newStateAsString;
    }
  }

  private calculateDuration( program: RunProgramState ) {
    let duration = 0;
    program.zones.forEach( ( zone ) => duration += zone.duration );
    program.duration = duration;
  }

  click( type: string, id: number, state: boolean ) {
    let component = this;
    this.pinService.setState( type, id, state ).subscribe( {
      complete() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error writing state', error );
      }
    } );
  }

  onSelectedProgram( programIndex: number ) {
    this.selectedProgramIndex = programIndex;
  }

  updateView() {
    this.pinUpdater.updateState();
    this.runUpdater.updateState();
  }

  startProgram() {
    console.log( "Starting program", this.selectedProgramIndex );
    if ( this.selectedProgramIndex === 0 ) {
      return;
    }
    let component = this;
    this.runService.start( this.selectedProgramIndex ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in startProgram', error );
      }
    } );
  }

  stopProgram() {
    let component = this;
    this.runService.stop().subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in stopProgram', error );
      },
    } );
  }

  nextZone() {
    let component = this;
    this.runService.nextZone().subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in nextZone', error );
      },
    } );
  }

  nextProgram() {
    let component = this;
    this.runService.nextProgram().subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in nextProgram', error );
      },
    } );
  }

  openSnackBar( log: string, error: any, message?: string ) {
    console.error( log, error );
    this.snackBar.openFromComponent(
      SnackbarErrorComponent,
      {
        data: message != undefined ? message : 'Hiba a kapcsolatban.',
        duration: 10000,
        horizontalPosition: 'right',
        panelClass: 'error-snackbar',
        verticalPosition: 'bottom',
      } );
  }

  toggleScheduledZoneState( program: RunProgramState, zone: RunZoneState ) {
    console.info( "toggle program " + program.id + " zone " + zone.index );
    let component = this;
    this.runService.toggleScheduledZoneState( program.id, zone.index ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in toggleScheduledZoneState', error );
      },
    } );
  }

  cancelSchedule( program: RunProgramState ) {
    console.info( "cancel schedule " + program.id );
    let component = this;
    this.runService.cancelSchedule( program.id ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.openSnackBar( 'Error in cancelSchedule', error );
      },
    } );
  }

  featureToggleZone(): boolean {
    return true;
  }
}
