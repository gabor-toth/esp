import { Component, OnInit, signal } from '@angular/core';
import { MatButton } from "@angular/material/button";
import { MatProgressSpinner } from "@angular/material/progress-spinner";
import { PinService } from "../pin/pin.service";
import { PinsConfiguration } from "../pin/pin";
import { RouterLink } from "@angular/router";

@Component( {
  selector: 'app-admin',
  templateUrl: './admin.component.html',
  imports: [
    MatButton,
    MatProgressSpinner,
    RouterLink
  ],
  styleUrls: [ './admin.component.scss' ]
} )
export class AdminComponent implements OnInit {
  readonly pins = signal<PinsConfiguration | undefined>( undefined );

  constructor( private pinService: PinService ) {
  }

  ngOnInit(): void {
    this.loadConfiguration();
  }

  private loadConfiguration() {
    this.pins.set( undefined );
    let component = this;
    this.pinService.getCachedOrLatestConfiguration( '' ).subscribe( {
      next( pinsConfiguration ) {
        component.pins.set( pinsConfiguration );
      },
      error( err ) {
        console.error( 'Error loading config', err );
      }
    } );
  }
}
