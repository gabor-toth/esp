import {Component, OnInit} from '@angular/core';
import {PinsState} from "../pin/pin";
import {StateService} from "./state.service";
import {PinService} from "../pin/pin.service";
import {animate, state, style, transition, trigger} from "@angular/animations";
import {ProgramService} from "../program/program.service";
import {RunService} from "../program/run.service";
import {RunState} from "../program/run";

@Component({
    selector: 'app-state',
    templateUrl: './state.component.html',
    styleUrls: ['./state.component.scss'],
    animations: [
        trigger('detailExpand', [
            state('collapsed', style({height: '0px', minHeight: '0'})),
            state('expanded', style({height: '*'})),
            transition('expanded <=> collapsed', animate('225ms cubic-bezier(0.4, 0.0, 0.2, 1)')),
        ]),
    ],
})
export class StateComponent implements OnInit {
    pinState: PinsState | undefined;
    runState: RunState | undefined;
    remoteTime: String | undefined;
    timer: number = 0;
    pinStateAsString = "";
    programStateAsString = "";

    constructor(private pinService: PinService, private runService: RunService) {
    }

    ngOnInit(): void {
        this.updateState();
        this.scheduleUpdate();
    }

    private scheduleUpdate() {
        if (this.timer) {
            clearTimeout(this.timer);
        }
        this.timer = setInterval(() => {
            this.updateState();
        }, 10000);
    }

    private onUpdatePinState(newState: PinsState) {
        this.remoteTime = newState.time?.time;
        newState.time = null;
        let newStateAsString = JSON.stringify(newState);
        if (newStateAsString != this.pinStateAsString) {
            this.pinState = newState;
            this.pinStateAsString = newStateAsString;
        }
    }

    private onUpdateRunState(newState: RunState) {
        let newStateAsString = JSON.stringify(newState);
        if (newStateAsString != this.programStateAsString) {
            this.runState = newState;
            this.programStateAsString = newStateAsString;
        }
    }

    private updateState() {
        let component = this;
        this.pinService.getState().subscribe({
            next(state) {
                component.onUpdatePinState(state);
            },
            error(err) {
                console.error('Error reading state', err);
            },
        });
        this.runService.getState().subscribe({
            next(state) {
                component.onUpdateRunState(state);
            },
            error(err) {
                console.error('Error reading state', err);
            },
        });
    }
}
