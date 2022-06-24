import {Component, OnInit} from '@angular/core';
import {State} from "./state";
import {StateService} from "./state.service";

@Component({
    selector: 'app-state',
    templateUrl: './state.component.html',
    styleUrls: ['./state.component.scss']
})
export class StateComponent implements OnInit {
    state: State | undefined;
    remoteTime: String | undefined;
    timer: number = 0;
    stateAsString = "";

    constructor(private stateService: StateService) {
    }

    ngOnInit(): void {
        this.updateState();
    }

    private scheduleUpdate() {
        if (this.timer) {
            clearTimeout(this.timer);
        }
        this.timer = setTimeout(() => {
            this.updateState();
        }, 5000);
    }

    private onUpdate(newState: State) {
        this.scheduleUpdate();
        this.remoteTime = newState.time?.time;
        newState.time = null;
        let newStateAsString = JSON.stringify(newState);
        if (newStateAsString != this.stateAsString) {
            this.state = newState;
            this.stateAsString = newStateAsString;
        }
    }

    private updateState() {
        clearTimeout(this.timer);
        this.timer = 0;
        let component = this;
        this.stateService.getState().subscribe({
            next(state) {
                component.onUpdate(state);
            },
            error(err) {
                component.scheduleUpdate();
                console.error('Error reading state', err);
            },
        });
    }

    click(type: String, id: number, state: boolean) {
        let component = this;
        this.stateService.setState(type, id, state).subscribe({
            complete() {
                component.updateState();
            },
            error(err) {
                console.error('Error writing state', err);
            }
        });
    }
}
