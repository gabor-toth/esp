import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AdminComponent } from './admin/admin.component';
import { PinComponent } from './pin/pin.component';
import { ProgramsComponent } from './program/programs.component';
import { StateComponent } from './state/state.component';

import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { FlexLayoutModule } from '@angular/flex-layout';
import { MatButtonModule } from "@angular/material/button";
import { MatIconModule } from "@angular/material/icon";
import { MatProgressSpinnerModule } from "@angular/material/progress-spinner";
import { MatTableModule } from "@angular/material/table";
import { MatFormField, MatLabel, MatOption, MatSelect } from "@angular/material/select";
import { MatSnackBarModule } from "@angular/material/snack-bar";
import { SnackbarErrorComponent } from "./common/snackbar-error/snackbar-error.component";

@NgModule( {
  declarations: [
    AdminComponent,
    PinComponent,
    ProgramsComponent,
    SnackbarErrorComponent,
    StateComponent,
  ],
  imports: [
    BrowserAnimationsModule,
    BrowserModule,
    FlexLayoutModule,
    // https://material.angular.io/cdk/layout/overview
    //LayoutModule,
    MatButtonModule,
    MatFormField,
    MatIconModule,
    MatLabel,
    MatOption,
    MatProgressSpinnerModule,
    MatSelect,
    MatSnackBarModule,
    MatTableModule,
  ],
  providers: [],
} )
export class AppModule {
}
