import { Injectable } from '@angular/core';
import { Program, ProgramDayType, ProgramDayValue, Programs } from './program';
import { map, Observable, of, switchMap } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { simulatedPrograms } from '../simulator/simulator';

@Injectable( {
  providedIn: 'root'
} )
export class ProgramService {
  constructor( private http: HttpClient ) {
  }

  getAll(): Observable<Program[]> {
    let service = this;
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms.programs );
    } else {
        return this.http.get<Programs>( environment.baseUrl + 'programs' ).pipe(
          map( programs => service.mapPrograms(programs))
        );
    }
  }

  private mapProgram( program: Program ) {
    program.days.type = <ProgramDayType><unknown>ProgramDayType[ program.days.type ];
    program.days.onDays?.map( value => {
      return ProgramService.mapDay( value );
    } );
    console.log( "intervalStartsOn", program.days.intervalStartsOn );
    if ( program.days.intervalStartsOn != undefined ) {
      console.log( program.days.intervalStartsOn, "-->", ProgramService.mapDay( program.days.intervalStartsOn ) );
      program.days.intervalStartsOn = ProgramService.mapDay( program.days.intervalStartsOn );
    } else {
      console.log( "xxx" );
    }
    return program;
  }

  private mapPrograms( programs: Programs ) {
    let service = this;
    programs.programs.forEach( ( program ) => {
      service.mapProgram( program );
    } );
    return programs.programs;
  }

  private static mapDay( value: ProgramDayValue ) {
    return <ProgramDayValue><unknown>ProgramDayValue[ value ];
  }

  get( index: number ): Observable<Program> {
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms.programs[ index ] );
    } else {
      let service = this;
      return this.http.get<Program>( environment.baseUrl + 'programs/' + index )
        .pipe(map( program => service.mapProgram(program) ) );
    }
  }

  set( program: Program ): Observable<Object> {
    let url = environment.baseUrl + 'programs';
    if ( program.index == 0 ) {
      return this.http.put( url, program );
    } else {
      return this.http.post( url, program );
    }
  }

  setEnabled( index: number, enabled: boolean ): Observable<Object> {
    return this.get( index ).pipe( switchMap( program => {
      program.enabled = enabled;
      return this.set( program );
    } ) );
  }
}
