import {Component} from '@angular/core';
// see https://fontawesome.com/icons/square-caret-down?s=solid
import {faSquareCaretDown, faSquareCaretUp, faToggleOff, faToggleOn} from '@fortawesome/free-solid-svg-icons';

import {StateService} from './state/state.service';
import {State} from './state/state';

@Component({
    selector: 'app-root',
    templateUrl: './app.component.html',
    styleUrls: ['./app.component.scss']
})
export class AppComponent {
    title = 'Öntöző';
    faToggleOff = faToggleOff
    faToggleOn = faToggleOn
    state: State | undefined;
    stateAsString = "";

    constructor(
        private stateService: StateService
    ) {

    }

    ngOnInit() {
        this.updateState();
    }

    private scheduleUpdate() {
        setTimeout(() => {
            this.updateState();
        }, 1000);
    }

    private onUpdate(newState: State) {
        this.scheduleUpdate();
        let newStateAsString = JSON.stringify(newState);
        if (newStateAsString != this.stateAsString) {
            this.state = newState;
            this.stateAsString = newStateAsString;
        }
    }

    private updateState() {
        let component = this;
        this.stateService.getState().subscribe({
            next(state) {
                component.onUpdate(state);
            },
            error(err) {
                component.scheduleUpdate();
                console.error('Error reading state', err);
            },
            // complete() {  }
        });
    }

    click(type: String, id: number, state: boolean) {
        console.log('Clicked ' + type + ' ' + id);
        let component = this;
        this.stateService.setState(type, id, state).subscribe({
            complete() {
                component.scheduleUpdate();
            },
            error(err) {
                console.error('Error writing state', err);
                component.scheduleUpdate();
            }
        });
    }
}
