import { Injectable } from '@angular/core';
import { PinsState } from './pin';
import { Observable } from "rxjs";
import { HttpClient } from '@angular/common/http';
import { environment } from '../../environments/environment';

@Injectable( {
  providedIn: 'root'
} )
export class PinService {

  timer: number = 0;

  constructor( private http: HttpClient ) {
  }

  getState(): Observable<PinsState> {
    return this.http.get<PinsState>( environment.baseUrl + 'state' );
  }

  setState( type: String, id: number, state: boolean ): Observable<Object> {
    let url = environment.baseUrl + type;
    let body = `{"id":${id}, "state": ${state}}`;
    return this.http.put( url, body );
  }
}
