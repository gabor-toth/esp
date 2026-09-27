import { Component, inject, OnInit, signal } from '@angular/core';
import { Program, ProgramDayType, ProgramDayValue } from "./program";
import { ProgramService } from "./program.service";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatCard, MatCardActions, MatCardContent, MatCardHeader, MatCardTitle } from "@angular/material/card";
import { MatButton } from "@angular/material/button";
import { SnackBar } from "../common/snackbar-error/snackbar";
import { RouterLink } from "@angular/router";
import { Confirmation } from "../common/confirmation/confirmation";

@Component( {
  selector: 'app-programs',
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
    MatButton,
    RouterLink
  ]
} )
export class ProgramsComponent implements OnInit {
  readonly programs = signal<Program[] | undefined>( undefined );
  readonly programDaysDisplay: string[] = [ 'H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V' ];
  readonly programDaysLongDisplay: string[] = [ 'hétfő', 'kedd', 'szerda', 'csütörtök', 'péntek', 'szombat', 'vasárnap' ];

  private readonly programService = inject( ProgramService );
  private readonly snackBar = inject( SnackBar );
  private readonly confirmation = inject( Confirmation );

  constructor() {
  }

  ngOnInit(): void {
    this.updateState();
  }

  private updateState() {
    let component = this;
    this.programService.getAll().subscribe( {
      next( programs ) {
        programs.sort( ( a, b ) => a.name.localeCompare( b.name ) );
        component.programs.set( programs );
      },
      error( error ) {
        component.snackBar.open( 'Error in updateState', error );
      },
    } );
  }

  hasDay( program: Program, dayIndex: number ): boolean {
    return program.days.onDays?.find( e => e == dayIndex ) != null;
  }

  setEnabled( program: Program, enabled: boolean ) {
    let component = this;
    this.programService.setEnabled( program.id, enabled ).subscribe( {
      next( object ) {
        component.updateState();
      },
      error( error ) {
        component.snackBar.open( 'Error in setEnabled', error, 'Nem sikerült a módosítás.' );
      }
    } );
  }

  delete( program: Program ) {
    let component = this;
    this.confirmation.open( `Biztosan törölni akarod a(z) "${ program.name }" programot?` ).subscribe( result => {
      this.programService.delete( program.id ).subscribe( {
        next( object ) {
          component.snackBar.message( 'Program sikeresen törölve.' );
          component.updateState();
        },
        error( error ) {
          component.snackBar.open( 'Nem sikerült a törlés.', error );
        }
      } );
    } );

  }

  protected readonly ProgramDayType = ProgramDayType;
}
