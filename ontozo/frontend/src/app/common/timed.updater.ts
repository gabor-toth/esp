import { Observable, Observer, Subscription } from "rxjs";

export abstract class TimedUpdater<T> {
  subscribers: Set<( Partial<Observer<T>> )> = new Set<( Partial<Observer<T>> )>();
  timer: number = 0;

  protected constructor() {
    this.scheduleUpdate();
  }

  protected scheduleUpdate() {
    if ( this.timer ) {
      clearTimeout( this.timer );
    }
    this.timer = window.setTimeout( () => {
      this.updateState();
    }, 5000 );
  }

  public subscribe( observer: Partial<Observer<T>> ): Subscription {
    this.subscribers.add( observer );
    if ( this.subscribers.size === 1 ) {
      this.updateState();
    }

    let component = this;
    return <Subscription>{
      unsubscribe() {
        component.subscribers.delete( observer );
        if ( component.subscribers.size === 0 ) {
          clearTimeout( component.timer );
        }
      }
    };
  }

  public updateState() {
    clearTimeout( this.timer );
    let component = this;
    this.getState().subscribe( {
      next( state ) {
        component.subscribers.forEach( ( e ) => e.next?.( state ) );
        component.scheduleUpdate();
      },
      error( err ) {
        component.scheduleUpdate();
        console.error( 'Error reading state', err );
      },
      complete() {
        component.subscribers.forEach( ( e ) => e.complete?.() );
      }
    } );
  }

  protected abstract getState(): Observable<T>;
}
