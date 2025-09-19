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
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms.programs );
    } else {
      return new Observable<Program[]>( ( subscriber ) => {
        this.http.get<Programs>( environment.baseUrl + 'programs' ).subscribe( {
          next( programs ) {
            programs.programs.forEach( ( program ) => {
              program.days.type = <ProgramDayType><unknown>ProgramDayType[ program.days.type ];
              program.days.onDays.map( value => {
                return <ProgramDayValue><unknown>ProgramDayValue[ value ];
              } );
            } )
            subscriber.next( programs.programs );
            subscriber.complete();
          },
          error( err ) {
            console.error( 'Error reading programs', err );
            subscriber.error( err );
          }
        } );
      } );
    }
  }

  get( index: number ): Observable<Program> {
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms.programs[ index ] );
    } else {
      return this.http.get<Program>( environment.baseUrl + 'programs/' + index );
    }
  }

  set( program: Program ): Observable<Object> {
    let url = environment.baseUrl + 'programs';
    return this.http.put( url, program );
  }

  setEnabled( index: number, enabled: boolean ): Observable<Object> {
    return this.get( index ).pipe( switchMap( program => {
      program.enabled = enabled;
      return this.set( program );
    } ) );
  }
}
