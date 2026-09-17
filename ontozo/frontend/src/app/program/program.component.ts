import { Component, inject, OnInit, signal } from '@angular/core';
import { Program, ProgramDay, ProgramDayType, ProgramDayValue, ProgramZone } from "./program";
import { ProgramService } from "./program.service";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatCard, MatCardActions, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";
import { MatButton } from "@angular/material/button";
import { SnackBar } from "../common/snackbar-error/snackbar";
import { ActivatedRoute, RouterLink } from "@angular/router";
import { FormBuilder, FormControl, FormsModule, ReactiveFormsModule, Validators } from "@angular/forms";
import { MatError, MatFormField, MatInput, MatLabel } from "@angular/material/input";
import { MatCheckbox } from "@angular/material/checkbox";
import { MatChipGrid, MatChipInput, MatChipInputEvent, MatChipRow, MatChipsModule } from "@angular/material/chips";

@Component( {
  selector: 'app-program',
  templateUrl: './program.component.html',
  styleUrls: [ './program.component.scss', './programs.component.scss' ],
  imports: [
    MatIcon,
    MatProgressSpinner,
    MatCardContent,
    MatButton,
    RouterLink,
    FormsModule,
    MatError,
    MatFormField,
    MatInput,
    MatLabel,
    ReactiveFormsModule,
    MatCheckbox,
    MatChipsModule,
  ]
} )
export class ProgramComponent implements OnInit {
  readonly program = signal<Program | undefined>( undefined );
  readonly keywords = signal<string[]>( [] );
  programDaysDisplay: string[] = [ 'H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V' ];
  id: number | null = null;

  private readonly activatedRoute = inject( ActivatedRoute );
  private readonly formBuilder = inject( FormBuilder );
  private readonly programService = inject( ProgramService );
  private readonly snackBar = inject( SnackBar );

  // workaround
  protected readonly ProgramDayType = ProgramDayType;

  nameFormControl = new FormControl( '', [ Validators.required ] );
  readonly settings = this.formBuilder.group( {
    active: false,
  } );

  readonly chipFormControl = new FormControl( '', [ Validators.pattern( /^[0-2]?[0-9]:[0-9]{2}$/ ) ] );

  constructor() {
    let id = this.activatedRoute.snapshot.params[ 'id' ];
    if ( id != null && id != "" ) {
      this.id = parseInt( id );
    }
  }

  ngOnInit(): void {
    this.updateState();
  }

  private updateState() {
    if ( this.id == null ) {
      return;
    }
    if ( this.id == 0 ) {
      this.program.set( {
        enabled: true,
        days: <ProgramDay>{
          type: <ProgramDayType><unknown>ProgramDayType[ ProgramDayType.onDays ],
          onDays: <ProgramDayValue[]><unknown>[]
        },
        index: 0,
        lastRunTime: 0,
        name: "",
        nextRunTime: 0,
        startTimes: [],
        valid: true,
        zones: <ProgramZone[]>[]
      } );
    } else {
      let component = this;
      this.programService.get( this.id ).subscribe( {
        next( program ) {
          component.program.set( program );
        },
        error( error ) {
          component.snackBar.open( 'Hiba a program betöltése közben', error );
        },
      } );
    }
    let program = this.program();
    if ( program ) {
      this.settings.controls.active.setValue( program.enabled );
    }
  }

  hasDay( program: Program, dayIndex: number ): boolean {
    let dayValue = ProgramDayValue[ dayIndex ];
    return program.days.onDays.find( e => e.valueOf().toString() == dayValue ) != null;
  }

  save( program: Program ): void {

  }

  removeKeyword( keyword: string ) {
    this.keywords.update( keywords => {
      const index = keywords.indexOf( keyword );
      if ( index < 0 ) {
        return keywords;
      }

      keywords.splice( index, 1 );
      return [ ...keywords ];
    } );
  }

  add( event: MatChipInputEvent ): void {
    // todo validateTime
    const value = ( event.value || '' ).trim();

    // Add our keyword
    if ( value ) {
      this.keywords.update( keywords => [ ...keywords, value ].sort(/*todo compareTime*/ ) );
    }

    // Clear the input value
    event.chipInput!.clear();
  }
}
