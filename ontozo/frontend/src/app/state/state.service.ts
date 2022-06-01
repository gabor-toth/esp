import {Injectable} from '@angular/core';
import {State} from './state';
import {Observable} from "rxjs";
import {HttpClient} from '@angular/common/http';

@Injectable({
    providedIn: 'root'
})
export class StateService {

    url = 'http://192.168.1.189/state';

    constructor(private http: HttpClient) {
    }

    getState(): Observable<State> {
        return this.http.get<State>(this.url);
    }

    getStateSimulate(): Observable<State> {
        const jsonString = `
{
  "outputs": {
    "pumps": [
      {
      "id": 1,
        "name": "öntöző",
        "on": false
      },
      {
      "id": 2,
        "name": "kút",
        "on": false
      }
    ],
    "zones": [
      {
      "id": 1,
        "name": "fű nagy",
        "on": false
      },
      {
            "id": 2,
        "name": "fű elől",
        "on": false
      },
      {
      "id": 3,
        "name": "fű hátul",
        "on": false
      },
      {
      "id": 4,
        "name": "---",
        "on": false
      },
      {
      "id": 5,
        "name": "---",
        "on": false
      },
      {
      "id": 6,
        "name": "---",
        "on": false
      },
      {
      "id": 7,
        "name": "---",
        "on": false
      },
      {
      "id": 8,
        "name": "---",
        "on": false
      }
    ]
  },
  "inputs": {
    "levels": [
      {
      "id": 1,
        "name": "level1",
        "on": true
      },
      {
      "id": 2,
        "name": "level2",
        "on": true
      },
      {
      "id": 3,
        "name": "level3",
        "on": true
      },
      {
      "id": 4,
        "name": "level4",
        "on": true
      }
    ],
    "buttons": [
      {
      "id": 1,
        "name": "button_start",
        "on": true
      },
      {
      "id": 2,
        "name": "button_stop",
        "on": true
      }
    ]
  }
}`;
        let jsonObj: any = JSON.parse(jsonString);
        return new Observable(subscriber => {
            // subscriber.next(<State>jsonObj);
            // subscriber.complete();
            setInterval(() => {
                subscriber.next(<State>jsonObj);
                // subscriber.complete();
            }, 1000);
        })
    }
}
