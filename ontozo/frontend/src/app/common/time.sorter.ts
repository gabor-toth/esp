import { Injectable } from "@angular/core";

@Injectable( {
  providedIn: 'root'
} )
export class TimeSorter {
  public sort( times: string[] ) {
    return times.sort( this.compareTime );
  }

  private compareTime( t1: string, t2: string ) {
    let v1 = t1.split( ':' ).map( s => parseInt( s ) );
    let v2 = t2.split( ':' ).map( s => parseInt( s ) );
    if ( v1.length != 2 || v2.length != 2 ) {
      console.error( "Wrong times", t1, t2 );
      return t1.localeCompare( t2 );
    }
    let result = v1[ 0 ] - v2[ 0 ];
    if ( result == 0 ) {
      result = v1[ 1 ] - v2[ 1 ];
    }
    return result;
  }
}