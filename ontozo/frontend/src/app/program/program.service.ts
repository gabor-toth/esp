import { inject, Injectable } from '@angular/core';
import { Program } from './program';
import { map, Observable, of, switchMap } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { simulatedPrograms } from '../simulator/simulator';
import { ProgramsWire, ProgramWire } from "./program.wire";
import { ProgramWireMapper } from "./program.wire.mapper";

@Injectable( {
  providedIn: 'root'
} )
export class ProgramService {
  private readonly http = inject( HttpClient );
  private readonly mapper = inject( ProgramWireMapper );

  constructor() {
  }

  getAll(): Observable<Program[]> {
    let service = this;
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms );
    } else {
      return this.http.get<ProgramsWire>( environment.baseUrl + 'programs' ).pipe(
        map( programs => service.mapper.mapProgramsFromWire( programs.programs ) )
      );
    }
  }

  get( index: number ): Observable<Program> {
    if ( environment.simulateRestCall ) {
      return of( simulatedPrograms[ index ] );
    } else {
      let service = this;
      return this.http.get<ProgramWire>( environment.baseUrl + 'programs/' + index )
        .pipe( map( program => service.mapper.mapProgramFromWire( program ) ) );
    }
  }

  set( program: Program ): Observable<Object> {
    let url = environment.baseUrl + 'programs';
    let programWire = this.mapper.mapProgramToWire( program );
    if ( program.index == 0 ) {
      return this.http.put( url, programWire );
    } else {
      return this.http.post( url, programWire );
    }
  }

  setEnabled( index: number, enabled: boolean ): Observable<Object> {
    return this.get( index ).pipe( switchMap( program => {
      program.enabled = enabled;
      return this.set( program );
    } ) );
  }
}
