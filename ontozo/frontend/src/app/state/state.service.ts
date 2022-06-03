import {Injectable} from '@angular/core';
import {State} from './state';
import {Observable} from "rxjs";
import {HttpClient} from '@angular/common/http';
import {environment} from '../../environments/environment';

@Injectable({
    providedIn: 'root'
})
export class StateService {

    constructor(private http: HttpClient) {
    }

    getState(): Observable<State> {
        return this.http.get<State>(environment.baseUrl + 'state');
    }

    setState(type: String, id: number, state: boolean): Observable<Object> {
        let url = environment.baseUrl + type;
        let body = `{"id":${id}, "state": ${state}}`;
        return this.http.put(url, body);
    }
}
