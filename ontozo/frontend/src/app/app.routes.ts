import { Routes } from '@angular/router';
import {StateComponent} from "./state/state.component";
import {PinComponent} from "./pin/pin.component";

export const routes: Routes = [
    {
        title: "Áttekintés",
        path: "dashboard",
        component: StateComponent
    }
    // {
    //     title: "Kapcsolótábla",
    //     path: "pin",
    //     component: PinComponent
    // }
];
