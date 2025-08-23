import { Injectable } from '@angular/core';
import { Observable } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { RunState } from "./run";
import { TimeoutInterceptor } from "../common/timeout.interceptor";
import {PinsState} from '../pin/pin';
import {simulatedPinState, simulatedPrograms} from '../simulator/simulator';

@Injectable( {
  providedIn: 'root'
} )
export class RunService {

  constructor( private http: HttpClient ) {
  }

  getState(): Observable<RunState> {
    if (environment.simulateRestCall) {
      return new Observable<RunState>((subscriber) => {
        subscriber.next(<RunState>{
          isProgramRunning: true,
          programs: [
            {
              index: simulatedPrograms[0].index,
              duration: 0,
              name: simulatedPrograms[0].name,
              running: true,
              zones: [
                {
                  index: simulatedPrograms[0].zones[0].zoneId,
                  duration: simulatedPrograms[0].zones[0].duration,
                  leftSeconds: 20,
                  name: simulatedPinState.outputs.zones[simulatedPrograms[0].zones[0].zoneId].name,
                  running: true
                },
                {
                  index: simulatedPrograms[0].zones[1].zoneId,
                  duration: simulatedPrograms[0].zones[1].duration,
                  leftSeconds: 20,
                  name: simulatedPinState.outputs.zones[simulatedPrograms[0].zones[1].zoneId].name,
                  running: false
                },
                {
                  index: simulatedPrograms[0].zones[2].zoneId,
                  duration: simulatedPrograms[0].zones[2].duration,
                  leftSeconds: 20,
                  name: simulatedPinState.outputs.zones[simulatedPrograms[0].zones[2].zoneId].name,
                  running: false
                }
              ]
            },
            {
              index: simulatedPrograms[1].index,
              duration: 0,
              name: simulatedPrograms[1].name,
              running: false,
              zones: [
                {
                  index: simulatedPrograms[1].zones[0].zoneId,
                  duration: simulatedPrograms[1].zones[0].duration,
                  name: simulatedPinState.outputs.zones[simulatedPrograms[1].zones[0].zoneId].name,
                }
              ]
            }
          ]
        });
      });
    } else {
      return this.http.get<RunState>( environment.baseUrl + 'run' );
    }
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
