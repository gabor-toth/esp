import { Component, inject, OnInit, signal, WritableSignal } from '@angular/core';
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
import { MatChipInputEvent, MatChipsModule } from "@angular/material/chips";
import { MatRadioChange, MatRadioModule } from "@angular/material/radio";
import { interval } from "rxjs";

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
    MatRadioModule,
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

  readonly chipFormControl = new FormControl( '', [ Validators.required, Validators.pattern( /^[0-2]?[0-9]:[0-9]{2}$/ ) ] );
  readonly selectedProgramDayType = signal<ProgramDayType | undefined>( undefined );
  readonly selectedDays = signal<number[]>( [] );

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
      let program = {
        enabled: true,
        days: <ProgramDay>{
          type: <ProgramDayType><unknown>ProgramDayType[ ProgramDayType.interval ],
          intervalDays: 3,
          // type: <ProgramDayType><unknown>ProgramDayType[ ProgramDayType.onDays ],
          // onDays: <ProgramDayValue[]><unknown>[]
        },
        index: 0,
        lastRunTime: 0,
        name: "",
        nextRunTime: 0,
        startTimes: [],
        valid: true,
        zones: <ProgramZone[]>[]
      };
      this.programLoaded( program );
    } else {
      let component = this;
      this.programService.get( this.id ).subscribe( {
        next( program ) {
          component.programLoaded( program );
        },
        error( error ) {
          component.snackBar.open( 'Hiba a program betöltése közben', error );
        },
      } );
    }
  }

  protected programLoaded( program: Program ) {
    this.program.set( program );
    this.settings.controls.active.setValue( program.enabled );
    let programDayType = parseInt( ProgramDayType[ program.days.type.valueOf() ] );
    this.selectedProgramDayType.set( programDayType );
  }

  hasDay( dayIndex: number ): boolean {
    return this.selectedDays().includes( dayIndex );
  }

  save(): void {

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

  onRadioChange( event: MatRadioChange ) {
    this.selectedProgramDayType.set( event.value );
  }

  protected onDayChange( dayIndex: number, hasDay: boolean ) {
    this.selectedDays.update( selectedDays => {
      let dayValue = ProgramDayValue[ dayIndex ];
      if ( hasDay ) {
        selectedDays = selectedDays.filter( e => e != dayIndex );
      } else {
        selectedDays.push( dayIndex );
      }
      return selectedDays;
    } );
  }
}
