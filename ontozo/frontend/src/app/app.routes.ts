import { Routes } from '@angular/router';
import { StateComponent } from "./state/state.component";
import { PinComponent } from "./pin/pin.component";
import { AdminComponent } from "./admin/admin.component";
import { ProgramsComponent } from "./program/programs.component";
import AdminPinComponent from "./admin/admin.pin.component";

export const routes: Routes = [
  { path: '', redirectTo: '/dashboard', pathMatch: 'full' },

  { path: "dashboard", component: StateComponent },
  { path: "pin", component: PinComponent },
  { path: 'program', component: ProgramsComponent },
  { path: 'admin', component: AdminComponent },
  { path: 'admin/pin/:type/:id', component: AdminPinComponent }
];
