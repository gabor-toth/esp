import { Component, OnInit } from '@angular/core';
import { PinsState } from "./pin";
import { PinService } from "./pin.service";
import { animate, state, style, transition, trigger } from "@angular/animations";
import { PinUpdater } from "./pin.updater";
import { Subscription } from "rxjs";
import {MatIcon} from '@angular/material/icon';
import {MatProgressSpinner} from '@angular/material/progress-spinner';
import {MatButton} from '@angular/material/button';
import {NgForOf, NgIf} from '@angular/common';

@Component({
  selector: 'app-pin',
  templateUrl: './pin.component.html',
  styleUrls: ['./pin.component.scss'],
  animations: [
    trigger('detailExpand', [
      state('collapsed', style({height: '0px', minHeight: '0'})),
      state('expanded', style({height: '*'})),
      transition('expanded <=> collapsed', animate('225ms cubic-bezier(0.4, 0.0, 0.2, 1)')),
    ]),
  ],
  imports: [
    MatIcon,
    MatProgressSpinner,
    MatButton,
    NgIf,
    NgForOf
  ]
})
export class PinComponent implements OnInit {
  state: PinsState | undefined;
  remoteTime: String | undefined;
  stateAsString = "";
  pinUpdaterSubscription?: Subscription;

  constructor( private pinService: PinService,
               private pinUpdater: PinUpdater ) {
  }

  ngOnInit(): void {
    this.subscribeForUpdates();
  }

  ngOnDestroy(): void {
    this.pinUpdaterSubscription?.unsubscribe();
  }

  private subscribeForUpdates() {
    let component = this;
    this.pinUpdaterSubscription = this.pinUpdater.subscribe( {
      next( state ) {
        component.onUpdate( state );
      },
    } );
  }

  private onUpdate( newState: PinsState ) {
    this.remoteTime = newState.time?.time;
    newState.time = null;
    let newStateAsString = JSON.stringify( newState );
    if ( newStateAsString != this.stateAsString ) {
      this.state = newState;
      this.stateAsString = newStateAsString;
    }
  }

  click( type: String, id: number, state: boolean ) {
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
