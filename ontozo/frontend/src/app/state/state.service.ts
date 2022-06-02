import {Injectable} from '@angular/core';
import {State} from './state';
import {Observable} from "rxjs";
import {HttpClient} from '@angular/common/http';

@Injectable({
    providedIn: 'root'
})
export class StateService {

    baseUrl = 'http://192.168.1.82/';

    constructor(private http: HttpClient) {
    }

    getState(): Observable<State> {
        return this.http.get<State>(this.baseUrl + 'state');
    }

    setState(type: String, id: number, state: boolean): Observable<Object> {
        let url = this.baseUrl + type;
        let body = `{"id":${id}, "state": ${state}}`;
        return this.http.put(url, body);
    }
}
