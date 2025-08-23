import {Injectable} from '@angular/core';
import {Program, ProgramDay, ProgramDayType, ProgramDayValue, ProgramShort, ProgramZone} from './program';
import {Observable} from "rxjs";
import {HttpClient} from '@angular/common/http';
import {environment} from '../../environments/environment';
import {simulatedPrograms} from '../simulator/simulator';

@Injectable({
  providedIn: 'root'
})
export class ProgramService {
  constructor(private http: HttpClient) {
  }

  getAll(): Observable<Program[]> {
    if (environment.simulateRestCall) {
      return new Observable<Program[]>((subscriber) => {
        subscriber.next(simulatedPrograms);
      });
    } else {
      return this.http.get<Program[]>(environment.baseUrl + 'programs');
    }
  }

  getShortInfo(): Observable<ProgramShort[]> {
    if (environment.simulateRestCall) {
      return new Observable<ProgramShort[]>((subscriber) => {
        subscriber.next(<ProgramShort[]>[
          {
            index: simulatedPrograms[0].index,
            name: simulatedPrograms[0].name
          },
          {
            index: simulatedPrograms[1].index,
            name: simulatedPrograms[1].name
          }
        ]);
      });
    } else {
      return this.http.get<ProgramShort[]>(environment.baseUrl + 'programs/short');
    }
  }

  get(index: number): Observable<Program> {
    if (environment.simulateRestCall) {
      return new Observable<Program>((subscriber) => {
        subscriber.next(simulatedPrograms[index]);
      });
    } else {
      return this.http.get<Program>(environment.baseUrl + 'programs/' + index);
    }
  }

  set(program: Program): Observable<Object> {
    let url = environment.baseUrl + 'programs';
    return this.http.put(url, program);
  }
}
