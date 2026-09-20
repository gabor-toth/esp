import { Component, computed, OnDestroy, OnInit } from '@angular/core';
import { PinsState } from "./pin";
import { PinService } from "./pin.service";
import { PinUpdater } from "./pin.updater";
import { UpdaterHandle } from "../common/timed.updater";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatButton, MatFabButton } from '@angular/material/button';
import { MatCard, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";

@Component( {
  selector: 'app-pin',
  templateUrl: './pin.component.html',
  styleUrls: [ './pin.component.scss' ],
  imports: [
    MatIcon,
    MatProgressSpinner,
    MatButton,
    MatFabButton,
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
  ]
} )
export class PinComponent implements OnInit, OnDestroy {
  private pinUpdaterHandle?: UpdaterHandle;

  constructor( private pinService: PinService,
               private pinUpdater: PinUpdater ) {
  }

  /** Polled pin state without `time`, so that the advancing clock alone does not redraw. */
  readonly state = computed<PinsState | undefined>( () => {
    let state = this.pinUpdater.state();
    return state === undefined ? undefined : { ...state, time: undefined };
  }, { equal: ( a, b ) => JSON.stringify( a ) === JSON.stringify( b ) } );

  readonly remoteTime = computed( () => this.pinUpdater.state()?.time?.time );

  ngOnInit(): void {
    this.pinUpdaterHandle = this.pinUpdater.watch();
  }

  ngOnDestroy(): void {
    this.pinUpdaterHandle?.unsubscribe();
  }

  click( type: string, id: number, state: boolean ) {
    let component = this;
    this.pinService.setState( type, id, state ).subscribe( {
      complete() {
        component.pinUpdater.updateState();
      },
      error( err ) {
        console.error( 'Error writing state', err );
      }
    } );
  }
}
