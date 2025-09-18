import { Injectable } from '@angular/core';
import { PinConfiguration, PinsConfiguration, PinsState, PinState } from './pin';
import { forkJoin, from, map, mergeMap, Observable, of, Subscriber, switchMap, tap } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';
import { simulatedPinConfiguration, simulatedPinState } from '../simulator/simulator';
import { PinStateAmender } from "./pin.state.amender";

@Injectable( {
  providedIn: 'root'
} )
export class PinService {

  private timer: number = 0;
  private cached: PinsConfiguration | undefined = undefined;

  constructor( private http: HttpClient,
               private pinStateAmender: PinStateAmender ) {
  }

  getCachedOrLatestConfiguration( version: string ): Observable<PinsConfiguration> {
    return this.cached !== undefined && this.cached.version === version ?
      of( this.cached ) :
      this.getConfiguration();
  }

  getState(): Observable<PinsState> {
    if ( environment.simulateRestCall ) {
      return of( simulatedPinState );
    }

    return this.http.get<PinsState>( environment.baseUrl + 'pins/state' )
      .pipe( switchMap(
        pinState => this.getCachedOrLatestConfiguration( pinState.version )
          .pipe( map(
            pinsConfiguration => this.pinStateAmender.process( pinsConfiguration, pinState )
          ) )
      ) );
  }

  setState( type: string, id: number, state: boolean ): Observable<Object> {
    let url = environment.baseUrl + `pins/${type}/${id}/${state ? "on" : "off"}`;
    return this.http.put( url, null );
  }

  getConfiguration(): Observable<PinsConfiguration> {
    if ( environment.simulateRestCall ) {
      return of( simulatedPinConfiguration );
    }
    return this.http.get<PinsConfiguration>( environment.baseUrl + 'pins' )
      .pipe( tap( pins => {
        this.cached = pins
      } ) );
  }

  setConfiguration( type: string, id: number, name: string ): Observable<Object> {
    let url = environment.baseUrl + type;
    let body = `{"id":${id}, "name": ${name}}`;
    return this.http.put( url, body );
  }

}
