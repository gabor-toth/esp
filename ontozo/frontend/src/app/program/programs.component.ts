import { Component, OnInit } from '@angular/core';
import { Program, ProgramDayType, ProgramDayValue } from "./program";
import {
  MatCell,
  MatCellDef,
  MatColumnDef,
  MatHeaderCell,
  MatHeaderCellDef,
  MatHeaderRow, MatHeaderRowDef,
  MatRow, MatRowDef,
  MatTable,
  MatTableDataSource,
} from '@angular/material/table';
import { ProgramService } from "./program.service";
import { animate, state, style, transition, trigger } from '@angular/animations';
import { RunService } from "./run.service";
import { MatIcon } from '@angular/material/icon';
import { MatProgressSpinner } from '@angular/material/progress-spinner';
import { MatButton, MatFabButton, MatIconButton } from '@angular/material/button';

@Component( {
  selector: 'app-program',
  templateUrl: './programs.component.html',
  styleUrls: [ './programs.component.scss' ],
  animations: [
    trigger( 'detailExpand', [
      state( 'collapsed', style( { height: '0px', minHeight: '0' } ) ),
      state( 'expanded', style( { height: '*' } ) ),
      transition( 'expanded <=> collapsed', animate( '225ms cubic-bezier(0.4, 0.0, 0.2, 1)' ) ),
    ] ),
  ],
  imports: [
    MatIcon,
    MatProgressSpinner,
    MatCellDef,
    MatColumnDef,
    MatHeaderCell,
    MatCell,
    MatIconButton,
    MatTable,
    MatButton,
    MatRow,
    MatFabButton,
    MatHeaderRow,
    MatHeaderCellDef,
    MatHeaderRowDef,
    MatRowDef
  ]
} )
export class ProgramsComponent implements OnInit {
  displayedColumns: string[] = [ 'name', 'enabled', 'start', 'expand' ];
  displayedColumnsMobile: string[] = [ 'name', 'enabled', 'expand' ];
  dataSource: MatTableDataSource<Program>;
  programs: Program[] | undefined;
  expandedElement: Program | null;
  programDaysDisplay: string[] = [ 'H', 'K', 'Sz', 'Cs', 'P', 'Sz', 'V' ];
  readonly ProgramDayType = ProgramDayType;

  constructor( private programService: ProgramService, private runService: RunService ) {
    this.dataSource = new MatTableDataSource( this.programs );
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
        component.dataSource = new MatTableDataSource( component.programs );
      },
      error( err ) {
        console.error( 'Error reading programs', err );
      },
    } );
  }

  clickExpand( element: Program ) {
    this.expandedElement = this.expandedElement === element ? null : element;
  }

  startProgram( element: Program ) {
    this.runService.start( element.index ).subscribe( {
      next() {
        // toaster: add success
      },
      error( err ) {
        // TODO toaster: add error
        console.error( 'Error starting program ' + element.index, err );
      },
    } );
  }

  stopProgram() {
    this.runService.stop().subscribe( {
      next() {
      },
      error( err ) {
        // TODO toaster: add error
        console.error( 'Error stopping program', err );
      },
    } );
  }

  nextZone() {
    this.runService.nextZone().subscribe( {
      next() {
      },
      error( err ) {
        // TODO toaster: add error
        console.error( 'Error moving to next zone', err );
      },
    } );
  }

  nextProgram() {
    this.runService.nextProgram().subscribe( {
      next() {
      },
      error( err ) {
        console.error( 'Error moving to next zone', err );
      },
    } );
  }

  hasDay( program: Program, dayIndex: number ): boolean {
    return program.days.onDays.find( e => e.valueOf().toString() == ProgramDayValue[ dayIndex ] ) != null;
  }
}
