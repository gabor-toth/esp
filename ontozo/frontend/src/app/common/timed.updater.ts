import { signal, Signal } from "@angular/core";
import { Observable } from "rxjs";

export interface UpdaterHandle {
  unsubscribe(): void;
}

/**
 * Polls the device every 5 seconds and publishes the result as a signal, so that
 * components reading it are refreshed even though they are `OnPush`.
 */
export abstract class TimedUpdater<T> {
  private readonly latestState = signal<T | undefined>( undefined );
  private watchers = 0;
  private timer: number = 0;

  /** Latest polled state, `undefined` until the first response arrives. */
  public readonly state: Signal<T | undefined> = this.latestState.asReadonly();

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

  /**
   * Registers interest in the polled state. Polling stops once every watcher has
   * released its handle.
   */
  public watch(): UpdaterHandle {
    this.watchers++;
    if ( this.watchers === 1 ) {
      this.updateState();
    }

    let component = this;
    let released = false;
    return {
      unsubscribe() {
        if ( released ) {
          return;
        }
        released = true;
        component.watchers--;
        if ( component.watchers === 0 ) {
          clearTimeout( component.timer );
          component.timer = 0;
        }
      }
    };
  }

  public updateState() {
    clearTimeout( this.timer );
    let component = this;
    this.getState().subscribe( {
      next( state ) {
        component.latestState.set( state );
        component.scheduleUpdate();
      },
      error( err ) {
        component.scheduleUpdate();
        console.error( 'Error reading state', err );
      }
    } );
  }

  protected abstract getState(): Observable<T>;
}
