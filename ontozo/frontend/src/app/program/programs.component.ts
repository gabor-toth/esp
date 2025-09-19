import { Component, OnInit } from '@angular/core';
import { Program, ProgramDayType, ProgramDayValue } from "./program";
import { ProgramService } from "./program.service";
import { RunService } from "./run.service";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatCard, MatCardActions, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";
import { MatButton } from "@angular/material/button";
import { SnackBar } from "../common/snackbar-error/snackbar";

@Component( {
  selector: 'app-program',
  templateUrl: './programs.component.html',
  styleUrls: [ './programs.component.scss' ],
  imports: [
    MatIcon,
    MatProgressSpinner,
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
    MatCardActions,
    MatButton
  ]
} )
export class ProgramsComponent implements OnInit {
  programs: Program[] | undefined;
  expandedElement: Program | null;
  programDaysDisplay: string[] = [ 'H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V' ];

  constructor( private programService: ProgramService,
               private runService: RunService,
               private snackBar: SnackBar ) {
    this.expandedElement = null;
  }

  ngOnInit(): void {
    this.updateState();
  }

  private updateState() {
    let component = this;
    this.programService.getAll().subscribe( {
      next( programs ) {
        component.programs = programs;
      },
      error( error ) {
        component.snackBar.open( 'Error in updateState', error );
      },
    } );
  }

  hasDay( program: Program, dayIndex: number ): boolean {
    let dayValue = ProgramDayValue[ dayIndex ];
    return program.days.onDays.find( e => e.valueOf().toString() == dayValue ) != null;
  }

  protected readonly ProgramDayType = ProgramDayType;

  setEnabled( program: Program, enabled: boolean ) {
    let component = this;
    this.programService.setEnabled( program.index, enabled ).subscribe( {
      next( object ) {
        component.updateState();
      },
      error( error ) {
        component.snackBar.open( 'Error in setEnabled', error );
      }
    } );
  }
}
