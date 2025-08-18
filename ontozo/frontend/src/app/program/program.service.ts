import { Injectable } from '@angular/core';
import { Program, ProgramShort } from './program';
import { Observable } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';

@Injectable( {
  providedIn: 'root'
} )
export class ProgramService {
  constructor( private http: HttpClient ) {
  }

  getAll(): Observable<Program[]> {
    return this.http.get<Program[]>( environment.baseUrl + 'programs' );
  }

  getShortInfo(): Observable<ProgramShort[]> {
    return this.http.get<ProgramShort[]>( environment.baseUrl + 'programs/short' );
  }

  get( index: number ): Observable<Program> {
    return this.http.get<Program>( environment.baseUrl + 'programs/' + index );
  }

  set( program: Program ): Observable<Object> {
    let url = environment.baseUrl + 'programs';
    return this.http.put( url, program );
  }
}
