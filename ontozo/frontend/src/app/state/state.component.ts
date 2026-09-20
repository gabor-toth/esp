import { Component, computed, OnDestroy, OnInit, signal } from '@angular/core';
import { PinsState } from "../pin/pin";
import { PinService } from "../pin/pin.service";
import { RunService } from "../program/run.service";
import { RunProgramState, RunZoneState } from "../program/run";
import { PinUpdater } from "../pin/pin.updater";
import { RunUpdater } from "../program/run.updater";
import { UpdaterHandle } from "../common/timed.updater";
import { ProgramService } from "../program/program.service";
import { DatePipe } from '@angular/common';
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatFormField } from '@angular/material/form-field';
import { MatOption, MatSelect } from '@angular/material/select';
import { MatButton, MatFabButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";
import { Program } from "../program/program";
import { SnackBar } from "../common/snackbar-error/snackbar";

/** Ignores identity changes so that an unchanged poll result does not redraw the cards. */
const sameJson = <T>( a: T, b: T ) => JSON.stringify( a ) === JSON.stringify( b );

@Component( {
  selector: 'app-state',
  templateUrl: './state.component.html',
  styleUrls: [ './state.component.scss' ],
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
    MatFabButton,
  ]
} )
export class StateComponent implements OnInit, OnDestroy {
  readonly programs = signal<Program[] | undefined>( undefined );
  readonly selectedProgramIndex = signal( 0 );

  private pinUpdaterHandle?: UpdaterHandle;
  private runUpdaterHandle?: UpdaterHandle;

  constructor( private snackBar: SnackBar,
               private pinService: PinService,
               private pinUpdater: PinUpdater,
               private programService: ProgramService,
               private runService: RunService,
               private runUpdater: RunUpdater ) {
  }

  /** Polled pin state without `time`, so that the advancing clock alone does not redraw. */
  readonly pinState = computed<PinsState | undefined>( () => {
    let state = this.pinUpdater.state();
    return state === undefined ? undefined : { ...state, time: undefined };
  }, { equal: sameJson } );

  readonly remoteTime = computed( () => this.pinUpdater.state()?.time?.time );

  /** Splits the polled run state into the running program and the queued ones. */
  private readonly runPrograms = computed( () => {
    let state = this.runUpdater.state();
    if ( state === undefined ) {
      return undefined;
    }
    if ( !state.isProgramRunning || state.programs.length === 0 ) {
      return { running: <RunProgramState>{}, queued: <RunProgramState[]>[] };
    }
    let [ running, ...queued ] = state.programs;
    return {
      running: this.withTotalDuration( this.trimToRunningZone( running ) ),
      queued: queued.map( ( program ) => this.withTotalDuration( program ) ),
    };
  }, { equal: sameJson } );

  readonly runState = computed( () => this.runPrograms()?.running );
  readonly queueState = computed( () => this.runPrograms()?.queued );

  ngOnInit(): void {
    this.pinUpdaterHandle = this.pinUpdater.watch();
    this.runUpdaterHandle = this.runUpdater.watch();
    this.loadPrograms();
  }

  ngOnDestroy(): void {
    this.pinUpdaterHandle?.unsubscribe();
    this.runUpdaterHandle?.unsubscribe();
  }

  private loadPrograms() {
    let component = this;
    this.programService.getAll().subscribe( {
      next( state ) {
        component.programs.set( state );
      },
      error( error ) {
        component.snackBar.open( 'Error loading programs', error );
      }
    } );
  }

  /** Drops the zones already finished and shows the running one's remaining time. */
  private trimToRunningZone( program: RunProgramState ): RunProgramState {
    let zones = program.zones ?? [];
    let runningIndex = zones.findIndex( ( zone ) => zone.running );
    if ( runningIndex < 0 ) {
      return { ...program, running: true };
    }
    return {
      ...program,
      running: true,
      zones: zones.slice( runningIndex )
        .map( ( zone, index ) => index === 0 ? { ...zone, duration: zone.leftSeconds } : zone ),
    };
  }

  private withTotalDuration( program: RunProgramState ): RunProgramState {
    let zones = program.zones ?? [];
    return {
      ...program,
      duration: zones.reduce( ( total, zone ) => total + zone.duration, 0 ),
    };
  }

  click( type: string, id: number, state: boolean ) {
    let component = this;
    this.pinService.setState( type, id, state ).subscribe( {
      complete() {
        component.updateView();
      },
      error( error ) {
        component.snackBar.open( 'Error writing state', error );
      }
    } );
  }

  onSelectedProgram( programIndex: number ) {
    this.selectedProgramIndex.set( programIndex );
  }

  updateView() {
    this.pinUpdater.updateState();
    this.runUpdater.updateState();
  }

  startProgram() {
    console.log( "Starting program", this.selectedProgramIndex() );
    if ( this.selectedProgramIndex() === 0 ) {
      return;
    }
    let component = this;
    this.runService.start( this.selectedProgramIndex() ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.snackBar.open( 'Error in startProgram', error );
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
        component.snackBar.open( 'Error in stopProgram', error );
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
        component.snackBar.open( 'Error in nextZone', error );
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
        component.snackBar.open( 'Error in nextProgram', error );
      },
    } );
  }

  toggleScheduledZoneState( program: RunProgramState, zone: RunZoneState ) {
    let component = this;
    this.runService.toggleScheduledZoneState( program.id, zone.index ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.snackBar.open( 'Error in toggleScheduledZoneState', error );
      },
    } );
  }

  cancelSchedule( program: RunProgramState ) {
    let component = this;
    this.runService.cancelSchedule( program.id ).subscribe( {
      next() {
        component.updateView();
      },
      error( error ) {
        component.snackBar.open( 'Error in cancelSchedule', error );
      },
    } );
  }

  featureToggleZone(): boolean {
    return true;
  }
}
