import { Component, inject, OnInit } from '@angular/core';
import { MatButton, MatButtonModule } from "@angular/material/button";
import { MatProgressSpinner } from "@angular/material/progress-spinner";
import { PinService } from "../pin/pin.service";
import { PinConfiguration, PinHelper, PinsConfiguration } from "../pin/pin";
import { ActivatedRoute, Router, RouterLink } from "@angular/router";
import { MatError, MatFormField, MatInput, MatLabel } from "@angular/material/input";
import { FormBuilder, FormControl, FormsModule, ReactiveFormsModule, Validators } from "@angular/forms";
import { MatCheckbox } from "@angular/material/checkbox";
import { SnackBar } from "../common/snackbar-error/snackbar";

@Component( {
  selector: 'app-admin-pin',
  templateUrl: './admin.pin.component.html',
  imports: [
    FormsModule,
    MatButton,
    MatButtonModule,
    MatProgressSpinner,
    RouterLink,
    MatFormField,
    MatLabel,
    ReactiveFormsModule,
    MatInput,
    MatError,
    MatCheckbox
  ],
  styleUrls: [ './admin.pin.component.scss' ]
} )
class AdminPinComponent implements OnInit {
  type: string | null = null;
  typeLabel: string | undefined = undefined;
  id: number | null = null;
  pin: PinConfiguration | undefined = undefined;
  pins: PinsConfiguration | undefined;
  nameFormControl = new FormControl( '', [ Validators.required ] );

  private readonly activatedRoute = inject( ActivatedRoute );
  private readonly formBuilder = inject( FormBuilder );
  private readonly pinService = inject( PinService );
  private readonly router = inject( Router );
  private readonly snackBar = inject( SnackBar );

  readonly settings = this.formBuilder.group( {
    active: false,
    hidden: false,
  } );

  // [errorStateMatcher]="matcher"

  constructor() {
    let id = this.activatedRoute.snapshot.params[ 'id' ];
    let type = this.activatedRoute.snapshot.params[ 'type' ];
    if ( id != null && id != "" && type != null && type != "" ) {
      this.id = parseInt( id );
      this.type = type;
    }
  }

  ngOnInit(): void {
    this.loadConfiguration();
  }

  private loadConfiguration() {
    this.pins = undefined;
    let component = this;
    this.pinService.getCachedOrLatestConfiguration( '' ).subscribe( {
      next( pinsConfiguration ) {
        component.pins = pinsConfiguration;
        component.selectPin();
      },
      error( err ) {
        console.error( 'Error loading config', err );
      }
    } );
  }

  private selectPin(): void {
    if ( this.id == undefined ) {
      return;
    }
    let pin;
    let typeLabel;
    switch ( this.type ) {
      case 'buttons':
        typeLabel = 'Gomb';
        pin = PinHelper.getPinById( this.pins?.inputs.buttons, this.id );
        break;
      case 'levels':
        typeLabel = 'Szint';
        pin = PinHelper.getPinById( this.pins?.inputs.levels, this.id );
        break;
      case 'pumps':
        typeLabel = 'Pumpa';
        pin = PinHelper.getPinById( this.pins?.outputs.pumps, this.id );
        break;
      case 'zones':
        typeLabel = 'Zóna';
        pin = PinHelper.getPinById( this.pins?.outputs.zones, this.id );
        break;
    }
    if ( pin == undefined ) {
      return;
    }
    this.typeLabel = typeLabel;
    this.pin = pin;
    this.nameFormControl.setValue( pin.name );
    this.settings.controls.active.setValue( !pin.inactive );
    this.settings.controls.hidden.setValue( pin.hidden );
    console.log(
      "selectPin type", typeLabel,
      "id", pin.id,
      "name", pin.name,
      " active", pin.inactive,
      " hidden", pin.hidden
    );
  }

  save() {
    let component = this;
    let name = this.nameFormControl.getRawValue();
    let active = this.settings.controls.active.getRawValue() || false;
    let hidden = this.settings.controls.hidden.getRawValue() || false;
    console.log( "save name", name,
      " active", active, this.settings.controls.active.getRawValue(), //this.settings.controls.active.value() != null ? this.settings.controls.active.value() : "null",
      " hidden", hidden
    );
    if ( name == null || this.type == null || this.id == null ) {
      return;
    }
    this.pinService.setConfiguration( this.type, this.id, name, !active, hidden ).subscribe( {
      complete() {
        component.router.navigate( [ "/admin" ] );
        component.snackBar.message( 'Sikeresen mentve.' );
      },
      error( err ) {
        component.snackBar.open( 'Failed to save pin config', err, 'Hiba a mentés során' );
      }
    } );
  }

}

export default AdminPinComponent
