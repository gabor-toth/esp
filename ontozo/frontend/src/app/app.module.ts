import {NgModule} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';

import {AppRoutingModule} from './app-routing.module';
import {AppComponent} from './app.component';

import {FontAwesomeModule} from '@fortawesome/angular-fontawesome';
import {HttpClientModule} from '@angular/common/http';
import {StateComponent} from './state/state.component';
import {AdminComponent} from './admin/admin.component';
import {ProgramComponent} from './program/program.component';

import {MatTableModule} from "@angular/material/table";
import {BrowserAnimationsModule} from '@angular/platform-browser/animations';
import {MatIconModule} from "@angular/material/icon";
import {MatButtonModule} from "@angular/material/button";
import {MatProgressSpinnerModule} from "@angular/material/progress-spinner";

@NgModule({
    declarations: [
        AppComponent,
        StateComponent,
        AdminComponent,
        ProgramComponent,
    ],
    imports: [
        BrowserModule,
        AppRoutingModule,
        FontAwesomeModule,
        HttpClientModule,
        BrowserAnimationsModule,
        MatTableModule,
        MatIconModule,
        MatButtonModule,
        MatProgressSpinnerModule,
    ],
    providers: [],
    bootstrap: [AppComponent]
})
export class AppModule {
}
