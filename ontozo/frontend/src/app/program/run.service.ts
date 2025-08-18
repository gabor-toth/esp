import { Injectable } from '@angular/core';
import { Observable } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { RunState } from "./run";
import { TimeoutInterceptor } from "../common/timeout.interceptor";

@Injectable( {
  providedIn: 'root'
} )
export class RunService {

  constructor( private http: HttpClient ) {
  }

  getState(): Observable<RunState> {
    return this.http.get<RunState>( environment.baseUrl + 'run' );
  }

  start( index: number ): Observable<Object> {
    let url = environment.baseUrl + 'run/start/' + index;
    return this.http.post( url, null );
  }

  stop(): Observable<Object> {
    let url = environment.baseUrl + 'run/stop';
    return this.http.post( url, null );
  }

  nextZone(): Observable<Object> {
    let url = environment.baseUrl + 'run/next/zone';
    return this.http.post( url, null );
  }

  nextProgram(): Observable<Object> {
    let url = environment.baseUrl + 'run/next/program';
    return this.http.post( url, null );
  }
}
