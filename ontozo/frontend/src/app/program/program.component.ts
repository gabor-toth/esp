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
import { MatOption, MatSelect } from "@angular/material/select";
import { MatListOption, MatSelectionList } from "@angular/material/list";

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
    MatSelect,
    MatOption,
    MatCard,
    MatCardHeader,
    MatCardTitle,
    MatSelectionList,
    MatListOption,
  ]
} )
export class ProgramComponent implements OnInit {
  readonly program = signal<Program | undefined>( undefined );
  readonly startTimes = signal<string[]>( [] );
  readonly programDaysDisplay: string[] = [ 'Hétfő', 'Kedd', 'Szerda', 'Csütörtök', 'Péntek', 'Szombat', 'Vasárnap' ];
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
  selectedProgramDayType = signal<ProgramDayType | undefined>( undefined );
  readonly selectedDays = signal<number[]>( [] );
  intervalDays = signal<number | undefined>( 3 );
  intervalStartsOn = signal<number | undefined>( 1 );
  intervalStartReset = signal<boolean>( false );

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
          type: ProgramDayType.interval,
          // type: ProgramDayType.onDays,
          intervalDays: 3,
          intervalStartsOn: 5,
          onDays: <ProgramDayValue[]><unknown>[ 1, 3, 5 ]
        },
        index: 0,
        lastRunTime: 0,
        name: "Teszt",
        nextRunTime: 0,
        startTimes: [ "11:00", "13:15" ],
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
    this.nameFormControl.setValue( program.name );
    this.settings.controls.active.setValue( program.enabled );
    let programDayType = program.days.type;
    this.startTimes.set(program.startTimes );
    this.selectedProgramDayType.set( programDayType );
    if ( programDayType == ProgramDayType.onDays ) {
      this.selectedDays.set( program.days.onDays || [] );
    } else {
      this.intervalDays.set( program.days.intervalDays );
      this.intervalStartsOn.set( program.days.intervalStartsOn );
      //this.intervalStartReset.set( program.intervalStartReset );
    }
  }

  hasDay( dayIndex: number ): boolean {
    return this.selectedDays().includes( dayIndex );
  }

  save(): void {
    let program = this.program();
    if ( program == undefined ) {
      return;
    }
    program.name = this.nameFormControl.getRawValue() || "";
    program.enabled = this.settings.controls.active.getRawValue() || false;
    //let programDayType = parseInt( ProgramDayType[ program.days.type.valueOf() ] );
    program.days.type = this.selectedProgramDayType() || ProgramDayType.unused;
    program.startTimes = this.startTimes();
    program.days.intervalDays = this.intervalDays() || 0;
    program.days.intervalStartsOn = this.intervalStartsOn() || -1;
    //mprogram.intervalStartReset = this.intervalStartReset();
    program.days.onDays = this.selectedDays() || [];

    let component = this;
    this.programService.set(program).subscribe( {
      next( program ) {
        component.snackBar.message( 'Program sikeresen mentve.' );
      },
      error( error ) {
        component.snackBar.open( 'Hiba a program mentése közben', error );
      },
    } );
  }

  removeKeyword( keyword: string ) {
    this.startTimes.update( keywords => {
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
      this.startTimes.update( keywords => [ ...keywords, value ].sort(/*todo compareTime*/ ) );
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
