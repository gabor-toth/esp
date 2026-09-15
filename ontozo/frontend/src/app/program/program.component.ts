import { Component, inject, OnInit } from '@angular/core';
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
    MatCheckbox
  ]
} )
export class ProgramComponent implements OnInit {
  program: Program | undefined;
  programDaysDisplay: string[] = [ 'H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V' ];
  id: number | null = null;

  private readonly activatedRoute = inject( ActivatedRoute );
  private readonly formBuilder = inject( FormBuilder );
  private readonly programService = inject( ProgramService );
  private readonly snackBar = inject( SnackBar );

  nameFormControl = new FormControl( '', [ Validators.required ] );
  readonly settings = this.formBuilder.group( {
    active: false,
  } );

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
      this.program = {
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
      };
    } else {
      let component = this;
      this.programService.get( this.id ).subscribe( {
        next( program ) {
          component.program = program;
        },
        error( error ) {
          component.snackBar.open( 'Hiba a program betöltése közben', error );
        },
      } );
    }
    if ( this.program ) {
      this.settings.controls.active.setValue( this.program.enabled );
    }
  }

  hasDay( program: Program, dayIndex: number ): boolean {
    let dayValue = ProgramDayValue[ dayIndex ];
    return program.days.onDays.find( e => e.valueOf().toString() == dayValue ) != null;
  }

  save( program: Program ): void {

  }

  protected readonly ProgramDayType = ProgramDayType;
}
