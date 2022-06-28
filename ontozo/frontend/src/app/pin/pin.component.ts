import {Component, OnInit} from '@angular/core';
import {PinsState} from "./pin";
import {PinService} from "./pin.service";
import {animate, state, style, transition, trigger} from "@angular/animations";

@Component({
    selector: 'app-state',
    templateUrl: './pin.component.html',
    styleUrls: ['./pin.component.scss'],
    animations: [
        trigger('detailExpand', [
            state('collapsed', style({height: '0px', minHeight: '0'})),
            state('expanded', style({height: '*'})),
            transition('expanded <=> collapsed', animate('225ms cubic-bezier(0.4, 0.0, 0.2, 1)')),
        ]),
    ],
})
export class PinComponent implements OnInit {
    state: PinsState | undefined;
    remoteTime: String | undefined;
    timer: number = 0;
    stateAsString = "";

    constructor(private stateService: PinService) {
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

    private onUpdate(newState: PinsState) {
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
