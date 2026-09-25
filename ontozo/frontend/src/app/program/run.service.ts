import { Injectable } from '@angular/core';
import { Observable, of } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { RunState } from "./run";
import { simulatedPinConfiguration, simulatedPrograms } from '../simulator/simulator';

@Injectable( {
  providedIn: 'root'
} )
export class RunService {

  constructor( private http: HttpClient ) {
  }

  getState(): Observable<RunState> {
    if ( environment.simulateRestCall ) {
      return of( <RunState>{
        isProgramRunning: true,
        programs: [
          {
            id: simulatedPrograms[ 0 ].id,
            duration: 0,
            name: simulatedPrograms[ 0 ].name,
            running: true,
            zones: [
              {
                id: simulatedPrograms[ 0 ].zones[ 0 ].id,
                duration: simulatedPrograms[ 0 ].zones[ 0 ].duration,
                leftSeconds: 20,
                name: simulatedPinConfiguration.outputs.zones[ simulatedPrograms[ 0 ].zones[ 0 ].id ].name,
                running: true
              },
              {
                id: simulatedPrograms[ 0 ].zones[ 1 ].id,
                duration: simulatedPrograms[ 0 ].zones[ 1 ].duration,
                leftSeconds: 20,
                name: simulatedPinConfiguration.outputs.zones[ simulatedPrograms[ 0 ].zones[ 1 ].id ].name,
                running: false
              },
              {
                id: simulatedPrograms[ 0 ].zones[ 2 ].id,
                duration: simulatedPrograms[ 0 ].zones[ 2 ].duration,
                leftSeconds: 20,
                name: simulatedPinConfiguration.outputs.zones[ simulatedPrograms[ 0 ].zones[ 2 ].id ].name,
                running: false
              }
            ]
          },
          {
            id: simulatedPrograms[ 1 ].id,
            duration: 0,
            name: simulatedPrograms[ 1 ].name,
            running: false,
            zones: [
              {
                id: simulatedPrograms[ 1 ].zones[ 0 ].id,
                duration: simulatedPrograms[ 1 ].zones[ 0 ].duration,
                name: simulatedPinConfiguration.outputs.zones[ simulatedPrograms[ 1 ].zones[ 0 ].id ].name,
              }
            ]
          }
        ]
      } );
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

  cancelSchedule( programId: number ): Observable<Object> {
    let url = environment.baseUrl + 'run/queued/' + programId;
    return this.http.delete( url );
  }

  toggleScheduledZoneState( programId: number, zoneIndex: number ): Observable<Object> {
    let url = environment.baseUrl + 'run/queued/' + programId + '/' + zoneIndex;
    return this.http.post( url, null );

  }
}
